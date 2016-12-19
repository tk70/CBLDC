/*
 * timer.h
 *
 * Created: 2014-12-18 22:20:26
 *  Author: Jakub Turowski
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#ifndef __ASSEMBLER__

#include "tools/atmega8_tp.h"

#define TIMER_PRESCALER 8
#define TIMER_MAX 32767
#define MS_TO_TICKS(ms) ((ms) * (F_CPU / 1000) / TIMER_PRESCALER)
#define US_TO_TICKS(us) ((us) * (F_CPU / 1000) / TIMER_PRESCALER / 1000)
#define TICKS_PER_SECOND (F_CPU/TIMER_PRESCALER)

static void timer_init()
{
	#if TIMER_PRESCALER == 1
		T1_PRES_1;
	#elif TIMER_PRESCALER == 8
		T1_PRES_8;
	#elif TIMER_PRESCALER == 64
		T1_PRES_64;
	#else	
	#error Prescaler?
	#endif
}

inline uint16_t timer_get()
{
	cli();
	uint16_t tcnt = TCNT1;
	sei();
	return tcnt;
}

inline uint8_t timer_ready(uint16_t ocr)
{
	cli();
	int16_t tcnt = TCNT1;
	sei();
	int16_t delta = ocr - tcnt;
	return delta < 0;
}

// *------------------*
// |      Timer A     |
// *------------------*

/* In some of these functions I use the OCR1 register, which may be confusing a little bit.
 In fact, I'm not using any timer interrupts, I just use this register to store timer set value.
 Using OCR1A/OCR1B register for this purpose is faster than RAM, since "in" and "out" are only 1 cycle
 and "sts" and "lds" are 2 cycles.
 
 Meh, not so sure about it now, since I have to do cli() and sei() every time since ZC interrupt uses TCNT1.
 */

// Store Absolute timer unlock time in OCR1A reg
inline void timerA_set(uint16_t time)
{
	cli();	
	OCR1A = time;
	sei();
}

inline uint16_t timerA_ocr()
{
	uint16_t ocr;
	cli();
	ocr = OCR1A;
	sei();
	return ocr;
}

// Store Relative (from now) timer unlock time in OCR1A reg
inline void timerA_set_rel(uint16_t time)
{
	cli();
	OCR1A = time + TCNT1;
	sei();
}

// Has the timer passed the value set earlier.
inline uint8_t timerA_ready()
{
	cli();											// cli() because timer reading is not atomic.
	int16_t ocr = OCR1A;							// Comparator interrupt service routine reads timer1 as well.
	int16_t tcnt = TCNT1;
	sei();
	int16_t delta = ocr - tcnt;
	return delta < 0;
}

inline int16_t timerA_remaining()
{
	cli();
	int16_t ocr = OCR1A;
	int16_t tcnt = TCNT1;
	sei();
	int16_t delta = ocr - tcnt;
	return delta;
}

inline void timerA_wait_ready()
{
	cli();
	int16_t ocr = OCR1A;
	int16_t tcnt;
	do {
		cli();
		tcnt = TCNT1;
		sei();
	} while ((ocr - tcnt) > 0);
}

inline void timerA_wait_until(uint16_t time)
{
	int16_t tcnt;
	do {
		cli();
		tcnt = TCNT1;
		sei();
	} while (((int16_t)time - tcnt) > 0);
}

void _noinline_timerA_wait_until(uint16_t time)
{
	timerA_wait_until(time);
}

// *------------------------*
// |      Timer A 32 bit    |
// *------------------------*
typedef union {
	struct {
		uint16_t l;
		uint16_t h;
	} lh;
		
	uint32_t t;
} t32;

struct {
	uint16_t cnt_x;
	uint16_t ocr_x;
} timerAX;

static uint32_t timerAX_get()
{
	t32 t;
	t.lh.l = timer_get();
	if ((int16_t)t.lh.l > 0 && BIS(TIFR, TOV1)) {
		timerAX.cnt_x++;
		SBI(TIFR, TOV1);
	}
	t.lh.h = timerAX.cnt_x;
	return t.t;
}

void timerAX_set(uint32_t time)
{
	timerA_set((uint16_t)time);
	timerAX.ocr_x = time >> 16;
}

//void timerAX_set_rel(uint32_t time)
//{
	//t32 t;
	//t.t = timerAX_get() + time;
	//timerAX_set(t.t);
//}

uint8_t timerAX_ready()
{
	t32 t;
	t32 ocr;
	t.t = timerAX_get();
	ocr.lh.l = timerA_ocr();
	ocr.lh.h = timerAX.ocr_x;
	int32_t delta = ocr.t - t.t;
	return delta < 0;
}

// *------------------*
// |      Timer B     |
// *------------------*

//void timerB_set(uint16_t time)
//{
	//cli();
	//OCR1B = time;
	//sei();
//}

void timerB_set_rel(uint16_t time)
{
	cli();
	OCR1B = time + TCNT1;
	sei();
}

inline uint8_t timerB_ready()
{
	cli();
	int16_t ocr = OCR1B;
	int16_t tcnt = TCNT1;
	sei();
	int16_t delta = ocr - tcnt;
	return delta < 0;
}

//int16_t timerB_remaining()
//{
	//cli();
	//int16_t ocr = OCR1B;
	//int16_t tcnt = TCNT1;
	//sei();
	//int16_t delta = ocr - tcnt;
	//return delta;
//}

#endif /* ASSEMBLER */

#endif /* TIMER_H_ */