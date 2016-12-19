/*
 * signal.s
 *
 * Created: 2014-12-26 01:46:33
 *  Author: Jakub Turowski
 *
 * ESC input signal module - ASM part.
 */ 

#include "signal.h"
#include "led.h"

#if INPUT_SIGNAL_TYPE == 1	

; External Interrupt service routine.
			
.global RC_PWM_INT
RC_PWM_INT:	in	isreg, _SFR_IO_ADDR(SREG)
		sbic	_SFR_IO_ADDR(RC_PWM_PIN), RC_PWM_P
		rjmp	rcp_rising

		; Falling RC PWM edge received
rcp_falling:	;sbrc	flagsA, RCP_EXPECTED_STATE
		;reti
		in	tmp_l, _SFR_IO_ADDR(TCNT1L)
		lds	tmp_h, rcp_rise_time
		sub	tmp_l, tmp_h
		sts	rcp_pulse_len, tmp_l
		in	tmp_l, _SFR_IO_ADDR(TCNT1H)
		lds	tmp_h, rcp_rise_time+1
		sbc	tmp_l, tmp_h
		sts	rcp_pulse_len+1, tmp_l
		;sbr	flagsA, 1<<RCP_EXPECTED_STATE
		sbr	flagsB, 1<<RCP_RECEIVED
		out	_SFR_IO_ADDR(SREG), isreg
		;LED1_0
		reti

		; Rising RC PWM edge received
rcp_rising:	;sbrs	flagsA, RCP_EXPECTED_STATE
		;reti
		in	tmp_l, _SFR_IO_ADDR(TCNT1L)
		in	tmp_h, _SFR_IO_ADDR(TCNT1H)
		sts	rcp_rise_time, tmp_l
		sts	rcp_rise_time+1, tmp_h
		;cbr	flagsA, 1<<RCP_EXPECTED_STATE
		out	_SFR_IO_ADDR(SREG), isreg
		;LED1_1
		reti



#endif  
; INPUT_SIGNAL_TYPE == 1









/*#include "signal.h"

#if RC_PWM_PIN_INTERRUPT == 1

#include "led.h"

.extern rcp_edge_time

	.global RC_PWM_INT
RC_PWM_INT:
		in	tmp_l, _SFR_IO_ADDR(TCNT1L)
		in	tmp_h, _SFR_IO_ADDR(TCNT1H)
		sts	rcp_edge_time, tmp_l
		sts	rcp_edge_time+1, tmp_h
		;DISABLE_INT
		;CLEAR_PENDING_INT
		reti

#endif  ; RC_PWM_PIN_INTERRUPT*/