/*
 * governor.h
 *
 * Created: 2015-01-03 21:12:51
 *  Author: Jakub Turowski
 * 
 * 
 * In governor mode, the ESC uses automatic throttle control in order to achieve setpoint speed.
 * We will be using a PID loop here.
 *
 *  P(t)       E(t)   -------------  U(t)   -------        S(t)
 *   --->--(X)---->--|  PI control |---->--| Motor |----.----->
 *      +   | -       -------------         -------     |
 *          |            ------------------             |
 *          *-----<-----| Power <-- Speed  |----<-------*
 *            F(t)       ------------------
 *
 * P - Power signal - direct PWM duty in normal mode, and "speed" desired on the motor, aka setpoint value
 *     in governor mode (calculated from incoming RCP/I2C signal)
 * S - Motor speed in Rounds Per Second read from the motor
 * F - Feedback - power signal value corresponding to S
 * E - Error value
 *
 *
 * The Governor mode uses the PID independent algorithm.
 *          .---[     P     ]---.
 *          |                   |
 * E(s)-->--.---[ 1/(Ti*s)  ]--(x)---> U(S)
 *          |                   |
 *          .---[    D*s    ]---.
 */ 


#ifndef GOVERNOR_H_
#define GOVERNOR_H_

#include "globals.h"
#include "signal.h"
#include "tools/atmega8_tp.h"

#define GOV_SAMPLE_INTERVAL (TICKS_PER_SECOND / GOV_SAMPLING_FREQ)
#define GOV_Ki (256000 / GOV_Ti / GOV_SAMPLING_FREQ)

#if GOV_Ti < 0xFFFF
	#if GOV_Ki < 2
		#error "GOV_Ki is close to zero, increase integration time?"
	#endif
#endif

uint16_t gov_power;						// Governor output power (U)
uint16_t gov_feedback;
int24_t gov_i;							// Integrator buffer
int16_t gov_error;

//int16_t gov_antiwindup_l;
//int16_t gov_antiwindup_h;
uint16_t gov_throttle_speed;
uint16_t gov_min_setpoint;
uint16_t gov_next_sample_time;
uint32_t gov_s2p_const;					// S(t) -> F(t) conversion constant


/*
 These calculations are quite long. I split them into feedback processing (motor speed to PID feedback conversion),
 error processing and actual PID algorithm.
*/

static void governor_process_feedback(uint16_t rps)
{
	gov_next_sample_time += GOV_SAMPLE_INTERVAL;
	/*
	 So we've got motor speed (rps) and value of max power signal (POWER_RANGE). First we need to convert the
	 speed value into corresponding power signal value: S(t) -> F(t).
	 Because speed desired on the motor is:
	 Sd(t) = P(t)/Pmax * Smax
	 power signal value (F) corresponding to motor speed can be calculated like this:
	 F(t) = S(t) * Pmax / Smax
	 Since only S(t) is variable here we can do
	 F(t) = S(t) * _gov_p2s_const / 2^24, where _gov_p2s_const = Pmax * (2^24-1) / Smax
	*/
	
	asm volatile (
	// 1. xA * mA
	"mul  %A1, %A2     \n\t"\
	"mov  r30, r1      \n\t"\
	// byte #1 (r0) skipped.

	// 2. xB * mA
	"mul  %B1, %A2     \n\t"\
	"clr  r31          \n\t"\
	"add  r30, r0      \n\t"\
	"adc  r31, r1      \n\t"\

	// 3. xA * mB
	"mul  %A1, %B2     \n\t"\
	"clr  %A0          \n\t"\
	"add  r30, r0      \n\t"\
	"adc  r31, r1      \n\t"\
	"adc  %A0, %A0     \n\t"\

	// byte #2 (r30) done.
	// 4. xB * mB
	"mul  %B1, %B2     \n\t"\
	"add  r31, r0      \n\t"\
	"adc  %A0, r1      \n\t"\

	// 5. xC * mA
	"mul  %C1, %A2     \n\t"\
	"clr  %B0          \n\t"\
	"add  r31, r0      \n\t"\
	"adc  %A0, r1      \n\t"\
	"adc  %B0, %B0     \n\t"\

	// byte #3 (r31) done.
	// 6. xC * mB
	"mul  %C1, %B2     \n\t"\
	"add  %A0, r0      \n\t"\
	"adc  %B0, r1      \n\t"\

	// 7. xD * mA
	"mul  %D1, %A2     \n\t"\
	"add  %A0, r0      \n\t"\
	"adc  %B0, r1      \n\t"\
	"brcs 1f           \n\t"\

	// byte #4 (%A0) done.
	// 8. xD * mB
	"mul  %D1, %B2     \n\t"\
	"tst  r1           \n\t"\
	"brne 1f           \n\t"\
	"add  %B0, r0      \n\t"\
	"brcc 2f           \n\t"\
	"1: ser %A0        \n\t"\
	"ser %B0           \n\t"\

	// byte #5 (%B0) done.
	"2: clr r1         \n\t"\

	: "=&d"(gov_feedback)					// result (d because "ser")
	: "r"(gov_s2p_const), "r"(rps)			// arguments
	: "r30", "r31"							// clobbered regs
	);
}

static void governor_process_error(uint16_t setpoint)
{
	// Keep the minimum speed
	if (setpoint && (setpoint < gov_min_setpoint)) {
		setpoint = gov_min_setpoint;
	}
	
	// U(t), controller output
	uint16_t u;
	
	// Calculate E(t), controller input
	int16_t error = setpoint - gov_feedback;
	error += gov_error;
	error /= 2;
	gov_error = error;
}

// Governor PID algorithm.
static void governor_process_pid()
{
	uint16_t u;
	// It was easier to write in asm, because I'm doing some 24 bit maths here with multiplications
	asm volatile (
	
#if GOV_Ti < 0xFFFF

	// Integrator. Calculate I block result.
	// Integrator buffer is kept in RAM as 24 bit variable. The LSB is here as a fractional part
	// used to increase the accuracy of integration, it's skipped after being stored in RAM again.
	// Load the integrator buffer to u.
	"lds   %A0, (gov_i)       \n\t"\
	"lds   %B0, (gov_i+1)     \n\t"\
	"lds   r31, (gov_i+2)     \n\t"\
	
	// u += error*GOV_Ki;
	"ldi   r18, %2            \n\t"\
	"mulsu %B1, r18           \n\t"\
	"add   %B0, r0            \n\t"\
	"adc   r31, r1            \n\t"\
	"mul   %A1, r18           \n\t"\
	"add   %A0, r0            \n\t"\
	"adc   %B0, r1            \n\t"\
	"clr   r1                 \n\t"\
	"adc   r31, r1            \n\t"\
	
	// Apply anti-windup to the integrator
	// if (u < 0) u = 0;
	"brpl  1f                 \n\t"\
	"clr   %A0                \n\t"\
	"clr   %B0                \n\t"\
	"clr   r31                \n\t"\
	"rjmp  2f                 \n\t"\

	// else if (ux:uh >= gov_antiwindup) u = gov_antiwindup<<8;
	"1:                      \n\t"\
	//"lds   r18, (gov_antiwindup_h)   \n\t"\
	//"lds   r19, (gov_antiwindup_h+1) \n\t"\
	
	"lds   r18, (pwm_range)   \n\t"\
	"lds   r19, (pwm_range+1) \n\t"\
	"cp    %B0, r18          \n\t"\
	"cpc   r31, r19          \n\t"\
	"brlo  2f                \n\t"\
	"mov   %B0, r18          \n\t"\
	"mov   r31, r19          \n\t"\
	"clr   %A0               \n\t"\
	"2:                      \n\t"\

	"sts  (gov_i),   %A0     \n\t"\
	"sts  (gov_i+1), %B0     \n\t"\
	"sts  (gov_i+2), r31     \n\t"\

	// u /= 256. Skip the least significant byte of the integration result.
	// It was used just as a fractional part.
	"mov   %A0, %B0          \n\t"\
	"mov   %B0, r31          \n\t"\
	"clr   r31               \n\t"\
	
#else

	"1:                      \n\t"\
	"2:                      \n\t"\
	"clr   %A0               \n\t"\
	"clr   %B0               \n\t"\
	"clr   r31               \n\t"\
	
#endif

	// Integration is done, now calculate P block result and add to u.
	// u += GOV_P * error
	"ldi   r18, %3           \n\t"\
	"mulsu %B1, r18          \n\t"\
	"add   %B0, r0           \n\t"\
	"adc   r31, r1           \n\t"\
	"mul   %A1, r18          \n\t"\
	"add   %A0, r0           \n\t"\
	"adc   %B0, r1           \n\t"\
	"clr   r1                \n\t"\
	"adc   r31, r1           \n\t"\
	
#if GOV_D

	// Calculate derivative of error (e')
	"lds	r18, (gov_error)     \n\t"\
	"lds	r19, (gov_error+1)   \n\t"\
	"sts	(gov_error),   %A1   \n\t"\
	"sts	(gov_error+1), %B1   \n\t"\
	
	"sub   %A1, r18          \n\t"\
	"sbc   %B1, r19          \n\t"\
	
	// Multiply e' * GOV_D and add to u
	"ldi   r18, %4           \n\t"\
	"mulsu %B1, r18          \n\t"\
	"add   %B0, r0           \n\t"\
	"adc   r31, r1           \n\t"\
	"mul   %A1, r18          \n\t"\
	"add   %A0, r0           \n\t"\
	"adc   %B0, r1           \n\t"\
	"clr   r1                \n\t"\
	"adc   r31, r1           \n\t"\
	
#endif

	"breq  4f                \n\t"\
	"brpl  3f                \n\t"\
	"clr   %A0               \n\t"\
	"clr   %B0               \n\t"\
	"rjmp  4f                \n\t"\

	// else if (u > 0xFFFF) u = 0xFFFF;
	"3:                      \n\t"\
	"ser   %A0               \n\t"\
	"ser   %B0               \n\t"\
	"4:                      \n\t"\

	: "=&r"(u)
	: "a"(gov_error), "M"(GOV_Ki), "M"(GOV_P), "M"(GOV_D)
	: "r18", "r19", "r31"
	);

	// Apply throttle rise slow-down
	if (u < gov_power) {
		gov_power = u;
	}
	else {
		gov_power += gov_throttle_speed;
		if (gov_power >= u) {
			gov_power = u;
		}
	}
}

static void governor_begin(uint16_t power)
{
	gov_next_sample_time = timer_get();
	gov_power = power;
	gov_i.l_hx.hx = power;
}

// PRE: config must be loaded
static void __attribute__((optimize("s"))) governor_init()
{
	if (BIS(cfg.flags, CFG_GOVERNOR)) {
		set_flag(flagsB, GOVERNOR);
		// Calculate governor constants
		//gov_antiwindup_h = (float)pwm_range * (0.01*GOV_ANTIWINDUP);
		////gov_antiwindup_l = -gov_antiwindup_h;
		float tmp = (float)pwm_range / (float)cfg.gov_max_rps;
		gov_min_setpoint = tmp * (float)RPM_TO_RPS(GOV_MIN_SPEED);
		gov_s2p_const = tmp * (float)0x01000000;
		gov_throttle_speed = signal_range * (0.01*GOV_THROTTLE_SPEED / GOV_SAMPLING_FREQ);
	}	
}

inline uint16_t governor_get_power()
{
	return gov_power;
}

inline uint8_t governor_needs_process()
{
	if (flag_is_set(flagsB, GOVERNOR)) {
		return timer_ready(gov_next_sample_time);
	}
	else {
		return 0;
	}	
}

#endif /* GOVERNOR_H_ */