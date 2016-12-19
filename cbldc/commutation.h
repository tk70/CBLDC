/*
 * commutation.h
 *
 * Created: 2014-12-29 21:01:29
 *  Author: Jakub Turowski
 */ 


#ifndef COMMUTATION_H_
#define COMMUTATION_H_

#include "comparator.h"
#include "pwm.h"

void (*commutation_ptr)(void) = 0;

// *------------------*
// |     Forward      |
// *------------------*

void comm_01();
void comm_12();
void comm_23();
void comm_34();
void comm_45();
void comm_50();

// R->T, S undriven. RH open, TL pwm
void comm_01()
{
	acomp_set_S();					// Set the B-EMF scan channel to S phase
	cli();							// Disable interrupts, don't want PWM generator to fire now
	SH_off();						// Close high S MOSFET in PWM Synchronous mode
	SL_off();						// Close low S MOSFET
	TL_pwm();						// Start PWM on low T MOSFET
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_12;		// Set pointer to the next commutation subroutine
}

// Commutation subroutines
// S->T, R undriven. SH open, TL pwm
void comm_12()
{
	acomp_set_R();
	cli();
	RH_off();
	SH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_23;
}

// S->R, T undriven. SH open, RL pwm
void comm_23()
{
	acomp_set_T();
	cli();
	TH_off();
	TL_off();
	RL_pwm();
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_34;
}

// T->R, S undriven. TH open, RL pwm
void comm_34()
{
	acomp_set_S();
	cli();
	SH_off();
	TH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_45;
}

// T->S, R undriven. TH open, SL pwm
void comm_45()
{
	acomp_set_R();
	cli();
	RH_off();
	RL_off();
	SL_pwm();
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_50;
}

// R->S, T undriven. RH open, SL pwm
void comm_50()
{
	acomp_set_T();
	cli();
	TH_off();
	RH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_01;
}

// *------------------*
// |     Backward     |
// *------------------*

void comm_10();
void comm_21();
void comm_32();
void comm_43();
void comm_54();
void comm_05();

// R->T, S undriven. RH open, TL pwm
void comm_10()
{
	acomp_set_S();
	cli();
	SH_off();
	RH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_05;
}

// Commutation subroutines
// S->T, R undriven. SH open, TL pwm
void comm_21()
{
	acomp_set_R();
	cli();
	RH_off();
	RL_off();
	TL_pwm();
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_10;
}

// S->R, T undriven. SH open, RL pwm
void comm_32()
{
	acomp_set_T();
	cli();
	TH_off();
	SH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_21;
}

// T->R, S undriven. TH open, RL pwm
void comm_43()
{
	acomp_set_S();
	cli();
	SH_off();
	SL_off();
	RL_pwm();
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_32;
}

// T->S, R undriven. TH open, SL pwm
void comm_54()
{
	acomp_set_R();
	cli();
	RH_off();
	TH_on();
	acomp_await_rising_zc();
	sei();
	commutation_ptr = comm_43;
}

// R->S, T undriven. RH open, SL pwm
void comm_05()
{
	acomp_set_T();
	cli();
	TH_off();
	TL_off();
	SL_pwm();
	acomp_await_falling_zc();
	sei();
	commutation_ptr = comm_54;
}

inline void commutate()
{
	(*commutation_ptr)();
}

inline void commutation_init()
{
	if (BIS(cfg.flags, CFG_DIRECTION)) {
		comm_10();
		comm_05();
	}
	else {
		comm_12();
		comm_23();
	}
}

#endif /* COMMUTATION_H_ */