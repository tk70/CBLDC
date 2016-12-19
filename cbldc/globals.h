/*
 * globals.h
 *
 * Created: 2014-12-18 22:39:17
 *  Author: Jakub Turowski
 */ 


#ifndef GLOBALS_H_
#define GLOBALS_H_
#ifdef __ASSEMBLER__

	#define flagsA r16
	#define flagsB r17
	#define pwm_low_l r6
	#define pwm_low_h r7
	#define pwm_high_l r8
	#define pwm_high_h r9
	#define tmp_l r10
	#define tmp_h r11
	#define pwm_tcnt2_h r12
	#define isreg r13
	//#define aco_samples r14
	
	#define TIMER2_OC_INT __vector_3

#else  /* !ASSEMBLER */

	#include <avr/io.h>
	#include <math.h>
	#include "bldc.h"
	#include "timer.h"
	#include "config.h"
	#include "tools/arithmetic.h"
	
	volatile register uint8_t flagsA asm("r16");
	volatile register uint8_t flagsB asm("r17");
	register uint8_t pwm_low_l asm("r6");
	register uint8_t pwm_low_h asm("r7");
	register uint8_t pwm_high_l asm("r8");
	register uint8_t pwm_high_h asm("r9");
	register uint8_t tmp_l asm("r10");
	register uint8_t tmp_h asm("r11");
	register uint8_t pwm_tcnt2_h asm("r12");
	register uint8_t isreg asm("r13");
	//register uint8_t aco_samples asm("r14");

	uint16_t signal_range;
	uint16_t pwm_range;
	uint16_t pwm_start_min;
	uint16_t pwm_start_max;
	
	uint8_t stp_mul;
	uint8_t stp_frac;
	uint8_t sttl_mul;
	uint8_t sttl_frac;
	
	float pwm_range_f;
	
	config cfg;

	// Atomic flag set, 1 bit
	inline void set_flag(uint8_t flagreg, uint8_t bit)
	{
		asm volatile (
			"ori %0, %1 \n\t"\
			: 
			: "a"(flagreg), "M"(1<<bit)
			: 
		);
	}
	
	// Atomic bit set, 2 bits
	inline void set_flags(uint8_t flagreg, uint8_t bit1, uint8_t bit2)
	{
		asm volatile (
		"ori %0, %1 \n\t"\
		:
		: "a"(flagreg), "M"((1<<bit1) + (1<<bit2))
		:
		);
	}
	
	// Atomic bit clear, 1 bit
	inline void clear_flag(uint8_t flagreg, uint8_t bit)
	{
		asm volatile (
		"andi %0, %1 \n\t"\
		:
		: "a"(flagreg), "M"(255 - (1<<bit))
		:
		);
	}
	
	// Atomic bit clear, 2 bits
	inline void clear_flags(uint8_t flagreg, uint8_t bit1, uint8_t bit2)
	{
		asm volatile (
		"andi %0, %1 \n\t"\
		:
		: "a"(flagreg), "M"(255 - (1<<bit1) - (1<<bit2))
		:
		);
	}
	
	inline uint8_t flag_is_set(uint8_t flagreg, uint8_t bit)
	{
		return BIS(flagreg, bit);
	}
	
	/* The function calculates some global constants. Things such as signal min and signal max are unknown
	in compilation time since user can calibrate RC PWM range etc. And since these are variables, I made
	PWM frequency a variable as well, since it's not much bigger problem now. At least our ESC will have
	a programmable PWM frequency, which is even better. I used the floating point calculations because some
	of them would be hard to do in _resonable_ way on fixed point operations, due to huge values involved.
	Let's say I didn't wanna waste yet another day on some assembly inlines. Governor initialization also
	needs some floating point calculations, so some functions were already compiled into the program bin.
	
	PRE: config must be loaded
	*/
	static void __attribute__((optimize("s"))) calculate_globals()
	{
		// Calculate input signal range (signal resolution)
		signal_range = cfg.rcp_high - cfg.rcp_low;
		
		// Calculate the Signal To PWM conversion constants, used later for conversion signal ->  throttle
		float pwm_period = (float)F_CPU / (float)cfg.pwm_freq;
		stp_mul = pwm_period / (float)signal_range;
		uint16_t tmp = pwm_period * 256.0 / (float)signal_range;
		tmp -= stp_mul<<8;
		stp_frac = tmp;
		
		// Calculate Speed To Throttle Limit conversion constants
		sttl_mul = (60.0*THROT_PER_KRPM/100000.0) * pwm_period;
		tmp = (60.0*THROT_PER_KRPM/100000.0*256.0) * pwm_period;
		tmp -= sttl_mul<<8;
		sttl_frac = tmp;
		
		// Power limit for startup
		pwm_start_min = pwm_period * (0.01*START_MIN_POWER);
		pwm_start_max = pwm_period * (0.01*START_MAX_POWER);
		
		// Construct the PWM range from the STP constants, making sure that the conversion at 100% signal
		// will always bring it to 100% throttle.
		pwm_range = mul_16_8_sum_frac8(signal_range, stp_mul, stp_frac);
	}
	
#endif /* ASSEMBLER */

// flagsA
#define PWM_BLINKING 0
#define PWM_STATE 1
#define PWM_R 2
#define PWM_S 3
#define PWM_T 4
#define PWM_SYNCHRO 5
#define SIGNAL_MAX 7


// flagsB
#define AWAIT_PRE_ZC 0
#define ZC_DETECTED 1
#define GOVERNOR 2
#define BRAKE 3
#define RCP_RECEIVED 4
#define SIGNAL_BRAKE 5
#define SIGNAL_RECEIVED 6
#define SIGNAL_ERROR 7

#endif /* GLOBALS_H_ */