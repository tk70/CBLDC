/*
 * beep.h
 *
 * Created: 2014-12-28 14:11:26
 *  Author: Jakub Turowski
 */ 


#ifndef BEEP_H_
#define BEEP_H_

#include "pwm.h"

// Beeper sound frequencies [Hz]
#define BEEP_C6		1047
#define BEEP_Db6	1108
#define BEEP_D6		1175
#define BEEP_Eb6	1245
#define BEEP_E6		1319
#define BEEP_F6		1397
#define BEEP_Gb6	1480
#define BEEP_G6		1568
#define BEEP_Ab6	1661
#define BEEP_A6		1760
#define BEEP_Bb6	1856
#define BEEP_B6		1976
#define BEEP_C7		2093

#define BEEP_C5		(BEEP_C6/2)
#define BEEP_Db5	(BEEP_Db6/2)
#define BEEP_D5		(BEEP_D6/2)
#define BEEP_Eb5	(BEEP_Eb6/2)
#define BEEP_E5		(BEEP_E6/2)
#define BEEP_F5		(BEEP_F6/2)
#define BEEP_Gb5	(BEEP_Gb6/2)
#define BEEP_G5		(BEEP_G6/2)
#define BEEP_Ab5	(BEEP_Ab6/2)
#define BEEP_A5		(BEEP_A6/2)
#define BEEP_Bb5	(BEEP_Bb6/2)
#define BEEP_B5		(BEEP_B6/2)

#define BEEP_HIGH_TIME 15		// [us]

#define F_TO_T(f) (F_CPU / f)

#if MS_TO_TICKS(10) >= TIMER_MAX
	#error Clock overflow for 10ms!
#endif

// Single beep sound
typedef struct {
	uint16_t period;			// Period (1/f_cpu)
	uint8_t time;				// Duration [cs] (centiseconds!)
} beep_snd;

// Beep melody
typedef struct {
	uint8_t n;					// Number of sounds
	beep_snd arr[];				// Array of sounds
} beep;

const beep beep_lost_signal = {
	n: 2,
	arr: {
		{F_TO_T(BEEP_B6), 20},
		{F_TO_T(BEEP_E6), 30}
	}
};

const beep beep_got_signal = {
	n: 2,
	arr: {
		{F_TO_T(BEEP_E6), 20},
		{F_TO_T(BEEP_B6), 30}
	}
};

const beep beep_prog_submenu = {
	n: 6,
	arr: {
		{F_TO_T(BEEP_Eb6), 8},
		{F_TO_T(BEEP_G6), 8},
		{F_TO_T(BEEP_Bb6), 8},
		{F_TO_T(BEEP_F6), 8},
		{F_TO_T(BEEP_A6), 8},
		{F_TO_T(BEEP_C7), 10},
	}
};

const beep beep_prog_success = {
	n: 5,
	arr: {
		{F_TO_T(BEEP_Bb5), 12},
		{F_TO_T(BEEP_D6), 12},
		{F_TO_T(BEEP_F6), 24},
		{F_TO_T(BEEP_A6), 26},
		{F_TO_T(BEEP_Bb6), 40},
	}
};

const beep beep_wdr = {
	n: 3,
	arr: {
		{F_TO_T(BEEP_E6), 10},
		{F_TO_T(BEEP_Ab6), 10},
		{F_TO_T(BEEP_B6), 15}
	}
};

const beep beep_start_fail = {
	n: 5,
	arr: {
		{F_TO_T(BEEP_F6), 20},
		{0, 20},
		{F_TO_T(BEEP_F6), 20},
		{0, 20},
		{F_TO_T(BEEP_F6), 20}
	}
};

// Play single sound
static void beep_sound(uint16_t period, uint8_t time)
{
	pwm_set(0);
	if (period) {
		pwm_set_top(period);
		pwm_set(BEEP_HIGH_TIME * (F_CPU / 1000000));
	}		
	do {
		_noinline_timerA_wait_until(timer_get() + MS_TO_TICKS(10));
	} while (--time);
	pwm_set(0);
	pwm_set_top(pwm_range);
}

// Play melody
static void beep_play(const beep* s)
{
	uint8_t n = s->n;
	const beep_snd* ptr = s->arr;
	do {
		uint16_t p = ptr->period;
		uint8_t t = ptr->time;
		ptr++;
		beep_sound(p, t);
	} while (--n);
}

#endif /* BEEP_H_ */