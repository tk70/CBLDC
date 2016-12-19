/*
/*
 * comparator.s
 *
 * Created: 2014-12-24 00:08:50
 *  Author: Jakub Turowski
 */ 

 #include "comparator.h"

 ; Analog comparator interrupt

.global ANA_COMP_INT
ANA_COMP_INT:	sbis	_SFR_IO_ADDR(ACSR), ACIS0
		rjmp	aco_falling

		; We were waiting for rising edge, and the interrupt has beed triggered.
		; Read the comparator state a few times doing other stuff in the meantime.
aco_rising:	sbis	_SFR_IO_ADDR(ACSR), ACO
		reti
		in	tmp_l,  _SFR_IO_ADDR(TCNT1L)		; Read ZC time
		in	tmp_h,  _SFR_IO_ADDR(TCNT1H)
		in	isreg, _SFR_IO_ADDR(SREG)
		
		sbis	_SFR_IO_ADDR(ACSR), ACO
		reti
		sts	_aco_zc_time, tmp_l
		sts	_aco_zc_time+1, tmp_h

		sbis	_SFR_IO_ADDR(ACSR), ACO
		reti

		sbrs	flagsB, AWAIT_PRE_ZC
		rjmp	aco_got_zc

aco_got_lh_pre_zc:
		cbr	flagsB, 1<<AWAIT_PRE_ZC			; We'll be waiting for actual ZC now.
		cbi	_SFR_IO_ADDR(ACSR), ACIS0
		out	_SFR_IO_ADDR(SREG), isreg
		reti

		; We were waiting for falling edge, and the interrupt has beed triggered.
aco_falling:	sbic	_SFR_IO_ADDR(ACSR), ACO
		reti
		in	tmp_l,  _SFR_IO_ADDR(TCNT1L)		; Read ZC time
		in	tmp_h,  _SFR_IO_ADDR(TCNT1H)
		in	isreg, _SFR_IO_ADDR(SREG)

		sbic	_SFR_IO_ADDR(ACSR), ACO
		reti
		sts	_aco_zc_time, tmp_l
		sts	_aco_zc_time+1, tmp_h

		sbic	_SFR_IO_ADDR(ACSR), ACO
		reti

		sbrs	flagsB, AWAIT_PRE_ZC
		rjmp	aco_got_zc

aco_got_hl_pre_zc:
		cbr	flagsB, 1<<AWAIT_PRE_ZC
		sbi	_SFR_IO_ADDR(ACSR), ACIS0
		out	_SFR_IO_ADDR(SREG), isreg
		reti

aco_got_zc:	cbi	_SFR_IO_ADDR(ACSR), ACIE		; ZC detected, we won't need any more interrupts.
		sbr	flagsB, (1<<ZC_DETECTED)
		out	_SFR_IO_ADDR(SREG), isreg
		reti