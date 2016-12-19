/*
 * comparator.h
 *
 * Created: 2014-12-18 21:39:14
 *  Author: Jakub Turowski
 */ 


#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include <avr/io.h>
#include "led.h"
#include "globals.h"

#ifdef __ASSEMBLER__
	#define ANA_COMP_INT __vector_16
	.extern _aco_zc_time;
	
#else

volatile uint16_t _aco_zc_time;

inline void acomp_init()
{
	SBI(SFIOR, ACME);
	SBI(ACSR, ACIS1);
}

inline void acomp_set_R()
{
	ADMUX = R_COMP_CHANNEL;
}

inline void acomp_set_S()
{
	ADMUX = S_COMP_CHANNEL;
}

inline void acomp_set_T()
{
	ADMUX = T_COMP_CHANNEL;
}

// Set detection for falling Zero-Cross.
// In fact set it to trigger on rising edge, detecting the PRE-ZC state
inline void acomp_await_falling_zc()
{
	CBI(ACSR, ACIE);
	SBI(ACSR, ACIS0);
}

// Set detection for rising Zero-Cross.
// In fact set it to trigger on falling edge, detecting the PRE-ZC state
inline void acomp_await_rising_zc()
{
	CBI(ACSR, ACIE);
	CBI(ACSR, ACIS0);
}

inline uint8_t acomp_state()
{
	return BIS(ACSR, ACO)? 1 : 0;
}

// *------------------*
// |    Zero-cross    |
// *------------------*

static void zc_run_begin()
{
	cli();
	SBI(ACSR, ACIE);								// Enable interrupt
	
	// What kind of PRE-ZC state are we waiting for now?
	if (BIS(ACSR, ACIS0)) {							// For high PRE-ZC					
		
		// Is it already a PRE-ZC state?
		if (BIS(ACSR, ACO)) {
			
			// Yes it is, await actual ZC now, set the interrupt on falling edge
			CBI(ACSR, ACIS0);						// Await falling ZC edge
			clear_flag(flagsB, AWAIT_PRE_ZC);
		} else {
			set_flag(flagsB, AWAIT_PRE_ZC);
		}
	}
	else {											// For low PRE-ZC	
		if (BIC(ACSR, ACO)) {
			SBI(ACSR, ACIS0);						// Await rising ZC edge
			clear_flag(flagsB, AWAIT_PRE_ZC);
		} else {
			set_flag(flagsB, AWAIT_PRE_ZC);
		}
	}
	clear_flag(flagsB, ZC_DETECTED);
	sei();	
}

// Has ZC been detected? For running mode.
inline uint8_t zc_run_detected()
{
	return flag_is_set(flagsB, ZC_DETECTED);
}

// Note: the ZC time variable is guaranteed to be correct only if ZC has been detected,
// (ZC_DETECTED flag set) otherwise it may be garbage.
inline uint16_t zc_run_time()
{
	return _aco_zc_time;
}

#endif // !__ASSEMBLER__

#endif /* COMPARATOR_H_ */