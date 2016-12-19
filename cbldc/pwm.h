/*
 * pwm.h
 *
 * Created: 2014-12-19 11:59:35
 *  Author: Jakub Turowski
 *
 *
 * PWM generation for power stage MOSFETs. This is mostly just C interface.
 * For performance reasons, the generator was implemented in ASM.
 *
 * There are 5 working modes.
 * - power off, PWM generator disabled - 0% throttle
 * - high-state-blinking mode - close to zero, but non-zero throttle
 * - normal mode
 * - low-state-blinking mode - close to max, but not max throttle
 * - power on, PWM generator disabled - 100% throttle
 * The problem with software generated PWM is that two interrupts can't fire at once
 * which causes power "bumps" around max and zero throttle.
 * In blinking mode, we use a single interrupt for generation of one PWM state.
 * We just switch the FETs, wait some time in the loop and switch them again.
*/ 


#ifndef PWM_H_
#define PWM_H_


#include "power_stage.h"
#include "globals.h"

#ifdef __ASSEMBLER__

#define _PWM_DEAD_CYCLES (F_CPU / 1000 * PWM_DEAD_TIME / 1000000)

.extern CONST_3

#else

#define _PWM_INT_EXEC_TIME 40

const uint8_t CONST_3 = 3;

uint16_t _pwm_top;
uint16_t _pwm_val;

inline uint16_t pwm_get_top()
{
	return _pwm_top;
}

inline void pwm_set_top(uint16_t top)
{
	_pwm_top = top;
}

void pwm_set(uint16_t duty)
{
	uint16_t hi, lo;
	hi = duty;
	_pwm_val = duty;
	lo = pwm_get_top() - duty;							// Calculate low state time.
	
	if (lo < _PWM_INT_EXEC_TIME) {
		// If the low state is less than full PWM interrupt execution clock cycles,
		// the PWM generator will be working in low-state-blinking mode.
		// The low state will be generated in one interrupt call.
		if (lo == 0) {
			// Max power, PWM off
			CBI(TIMSK, OCIE2);							// Disable the PWM generator
			set_flag(flagsA, PWM_STATE);
			if (flag_is_set(flagsA, PWM_S))				// Set low state on FETs
				SL_on();
			if (flag_is_set(flagsA, PWM_R))						
				RL_on();
			if (flag_is_set(flagsA, PWM_T))
				TL_on();
		} else {
			// Low-state-blinking mode
			cli();
			set_flags(flagsA, PWM_BLINKING, PWM_STATE);
			pwm_low_l = (uint8_t)(pwm_get_top());		// Set full cycle time as duty
			pwm_low_h = (uint8_t)(pwm_get_top()>>8);
			pwm_high_l = lo-1;
			sei();
			SBI(TIMSK, OCIE2);							// Enable the PWM generator
		}
	} else
	
	if (hi < _PWM_INT_EXEC_TIME) {
		// Zero power, PWM off
		if (hi == 0) {
			CBI(TIMSK, OCIE2);							// Disable the PWM generator
			clear_flag(flagsA, PWM_STATE);
			SL_off();									// Set low state on FETs
			RL_off();
			TL_off();
		} else {
			// High-state-blinking mode
			cli();
			set_flag(flagsA, PWM_BLINKING);
			clear_flag(flagsA, PWM_STATE);
			pwm_high_l = (uint8_t)(pwm_get_top());		// Set full cycle time as duty
			pwm_high_h = (uint8_t)(pwm_get_top()>>8);
			pwm_low_l = hi-1;							// Set blink time
			sei();
			SBI(TIMSK, OCIE2);							// Enable the PWM generator
		}
	}
	
	// Normal PWM mode. There are separate interrupts for low and high states.
	else {
		cli();
		clear_flag(flagsA, PWM_BLINKING);
		pwm_low_l = (uint8_t)(lo);						// Set low and high state times
		pwm_low_h = (uint8_t)(lo>>8);
		pwm_high_l = (uint8_t)(hi);
		pwm_high_h = (uint8_t)(hi>>8);
		sei();
		SBI(TIMSK, OCIE2);								// Enable the PWM generator
	}
}

static void pwm_init()
{
	OCR2 = 255;
	pwm_set_top(pwm_range);
	pwm_set(0);
	clear_flag(flagsA, PWM_SYNCHRO);
	if (BIS(cfg.flags, CFG_SYNCHRO_PWM)) set_flag(flagsA, PWM_SYNCHRO);
	TCCR2 = (1<<CS20);	// timer2: prescaler 0
}

inline uint16_t pwm_get()
{
	return _pwm_val;
}

inline void SL_pwm()
{
	if (flag_is_set(flagsA, PWM_STATE)) {
		SL_on();
	}	
	set_flag(flagsA, PWM_S);
	clear_flags(flagsA, PWM_R, PWM_T);
}

inline void RL_pwm()
{
	if (flag_is_set(flagsA, PWM_STATE)) {
		RL_on();
	}	
	set_flag(flagsA, PWM_R);
	clear_flags(flagsA, PWM_S, PWM_T);
}

inline void TL_pwm()
{
	if (flag_is_set(flagsA, PWM_STATE)) {
		TL_on();
	}
	clear_flags(flagsA, PWM_S, PWM_R);
	set_flag(flagsA, PWM_T);
}

#endif /* ASSEMBLER */

#endif /* PWM_H_ */