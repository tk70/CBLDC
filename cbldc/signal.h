/*
 * signal.h
 *
 * Created: 2014-12-19 14:15:12
 *  Author: Jakub Turowski
 */ 


#ifndef SIGNAL_H_
#define SIGNAL_H_

#include "bldc.h"
#include "globals.h"

#if INPUT_SIGNAL_TYPE == 1

	#define RC_PWM_PORT	PORTD
	#define RC_PWM_PIN	PIND
	#define RC_PWM_DDR	DDRD
	
	#if RC_PWM_CHANNEL == 0
		#define RC_PWM_P 2
		#define ISCx0_BIT	ISC00
		#define ISCx1_BIT	ISC01
		#define INTx_BIT	INT0
		#define INTFx_BIT	INTF0
		
	#elif RC_PWM_CHANNEL == 1
		#define RC_PWM_P	3
		#define ISCx0_BIT	ISC10
		#define ISCx1_BIT	ISC11
		#define INTx_BIT	INT1
		#define INTFx_BIT	INTF1
		
	#else
		#error Invalid constant: RC_PWM_CHANNEL. Please select 0 (external interrupt 0) or 1 (external interrupt 1)
	#endif

#elif INPUT_SIGNAL_TYPE == 2
	#error I2C not implemented yet!
#else
	#error Invalid constant: INPUT_SIGNAL_TYPE. Please select 1 (RC PWM) or 2 (I2C)
#endif


#ifdef __ASSEMBLER__
// *-----------------------------------------------------------------------------------*
// |                                      ASM                                          |
// *-----------------------------------------------------------------------------------*

#if INPUT_SIGNAL_TYPE == 1
	
	.extern rcp_rise_time
	.extern rcp_pulse_len
	
	#if RC_PWM_CHANNEL == 0
		#define RC_PWM_INT __vector_1
	#else
		#define RC_PWM_INT __vector_2
	#endif
	
#elif INPUT_SIGNAL_TYPE == 2
#else
#endif

#else // __ASSEMBLER__

// *-----------------------------------------------------------------------------------*
// |                                       C                                           |
// *-----------------------------------------------------------------------------------*
#if MS_TO_TICKS(10) >= TIMER_MAX
	#error Clock overflow for 10ms!
#endif

#include "timer.h"
#include "tools/arithmetic.h"

uint16_t _signal_val;
uint8_t _signal_timeout;

// Incoming signal range depends on incoming signal type, and power stage PWM range
// depends on desired PWM frequency.
// This function converts ANY incoming signal range (RC PWM, I2C) into ANY PWM range.
// It uses hardware-implemented "mul", so it's fast.
// We have two constants, multiplier and fraction.
// PWM = (sig * multiplier) + (sig * fraction) / 256.
// See description of these constants in globals.h
inline uint16_t __signal_to_pwm_range(uint16_t sig)
{
	uint16_t result = mul_16_8_sum_frac8(sig, stp_mul, stp_frac);
	return result;
}

#if INPUT_SIGNAL_TYPE == 1
	// *-------------------------------------------------------------------------------*
	// |                                  RC PWM                                       |
	// *-------------------------------------------------------------------------------*
	
	uint16_t rcp_rise_time;
	uint16_t rcp_pulse_len;
	
	void __rcp_err()
	{
		_signal_val = 0;
		set_flags(flagsB, SIGNAL_ERROR, SIGNAL_RECEIVED);
		if (flag_is_set(flagsB, BRAKE)) {
			set_flag(flagsB, SIGNAL_BRAKE);
		}
	}	
	
	// PRE: config must be loaded
	static void signal_init()
	{
		CBI(RC_PWM_DDR, RC_PWM_P);							// RCP pin as input
		_signal_val = 0;
		cli();
		SBI(GICR, INTx_BIT);
		SBI(MCUCR, ISCx0_BIT);
		SBI(GIFR, INTFx_BIT);
		sei();
		timerB_set_rel(MS_TO_TICKS(10));
	}

	void signal_process()
	{
		
		if (flag_is_set(flagsB, RCP_RECEIVED)) {
			cli();
			clear_flag(flagsB, RCP_RECEIVED);			
			uint16_t len = rcp_pulse_len;
			sei();				
			if ((len >= cfg.rcp_min)) {
				clear_flag(flagsA, SIGNAL_MAX);
				if (len >= cfg.rcp_max) {
					set_flag(flagsA, SIGNAL_MAX);
				}
				clear_flags(flagsB, SIGNAL_ERROR, SIGNAL_BRAKE);
				int16_t time = len - cfg.rcp_low;
				if (time < 0) {
					if (flag_is_set(flagsB, BRAKE) && time < -((int16_t)US_TO_TICKS(RC_PWM_BRAKE_THRESHOLD))) {
						set_flag(flagsB, SIGNAL_BRAKE);
					}
					time = 0;
				}
				else if (time >= signal_range) time = signal_range;
				uint16_t power = __signal_to_pwm_range(time);
				_signal_val = power;
			}
			else {
				__rcp_err();

			}
			_signal_timeout = RC_PWM_TIMEOUT;
			set_flag(flagsB, SIGNAL_RECEIVED);
			timerB_set_rel(MS_TO_TICKS(10));
		}
		else if (timerB_ready()) {
			if (--_signal_timeout == 0) {
				__rcp_err();
			}
			timerB_set_rel(MS_TO_TICKS(10));
		}
	}

#elif INPUT_SIGNAL_TYPE == 2
	// *-------------------------------------------------------------------------------*
	// |                                    I2C                                        |
	// *-------------------------------------------------------------------------------*
	
	void signal_init()
	{
	}

	uint8_t signal_process()
	{
	}
	
#else
#endif
	
inline uint8_t signal_error()
{
	return flag_is_set(flagsB, SIGNAL_ERROR);
}

inline uint8_t signal_brake()
{
	return flag_is_set(flagsB, SIGNAL_BRAKE);
}

inline uint8_t signal_max()
{
	return flag_is_set(flagsA, SIGNAL_MAX);
}

inline uint16_t signal_get_power()
{
	return _signal_val;
}

#endif /* !__ASSEMBLER__ */
#endif /* SIGNAL_H_ */