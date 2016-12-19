/*
 * pwm.s
 *
 * Created: 2014-12-19 00:08:50
 *  Author: Jakub Turowski
 * PWM generation for power stage MOSFETs - ASM implementation.
 */ 

 #include "pwm.h"
 #include "led.h"

/*.global TIMER2_OC_INT
dead_time_delay:
	.if _PWM_DEAD_CYCLES-7 >= 0
		.if (_PWM_DEAD_CYCLES-7) & 1
			nop
		.endif
		.if (_PWM_DEAD_CYCLES-7) & 2
			rjmp	.+0
		.endif
		.if (_PWM_DEAD_CYCLES-7) & 4
			rjmp	.+0
			rjmp	.+0
		.endif
		.if (_PWM_DEAD_CYCLES-7) & 8
			rcall	dtd_ret
			nop
		.endif
		.if (_PWM_DEAD_CYCLES-7) & 16
			rcall	dtd_ret
			rcall	dtd_ret
			rjmp	.+0
		.endif
		.if _PWM_DEAD_CYCLES >= 32
			.error	"Reduce PWM_DEAD_TIME"
		.endif
	.endif*/

.if _PWM_DEAD_CYCLES-6 >= 32
	.error	"Reduce PWM_DEAD_TIME"
.endif

dtd_ret:	ret
 ; Timer 2 output compare interrupt service routine
 
.global TIMER2_OC_INT
TIMER2_OC_INT:	in	isreg, _SFR_IO_ADDR(SREG)	; Store SREG
		dec	pwm_tcnt2_h
		brpl	pwm_ret				; If tcnt2 extended byte was still non-zero, return

		bst	flagsA, PWM_STATE
		brts	pwm_set_low

		; If it was low PWM state, and we will be doing high state now.
pwm_set_high:	in	tmp_l, _SFR_IO_ADDR(OCR2)	; Calculate and set time of the next low state
		add	tmp_l, pwm_high_l
		out	_SFR_IO_ADDR(OCR2), tmp_l
		mov	pwm_tcnt2_h, pwm_high_h
		
		; If the low byte of delay was small enough, we could miss one interrupt when we incremented OCR2A,
		; because TCNT2 could have already exceeded new value of OCR2A.
		; To prevent this, we check if we have missed one interrupt, by analysing low byte of delay and comparing
		; comparing TCNT2 and OCR2A.
		tst	pwm_high_l			; Was the next interrupt supposed to hit in less than 128 cycles?
		;in	tmp_l, _SFR_IO_ADDR(OCR2)	;  *These two lines could be after brmi, just wanted to reduce dependence
		in	tmp_h, _SFR_IO_ADDR(TCNT2)	;   between FET turn-on time and program execution path.
		brmi	pwm_tcl_done			; No.
		sub	tmp_l, tmp_h			; Yes. OCR2 -= TCNT2. Is TCNT2 already greater than OCR2A?
		brpl	pwm_tcl_done			; No.

		clr	tmp_l				; Clear interrupt in case we actually didn't miss it and it's pending.
		set
		bld	tmp_l, OCF2
		out	_SFR_IO_ADDR(TIFR), tmp_l
		dec	pwm_tcnt2_h

pwm_tcl_done:	bst	flagsA, PWM_BLINKING
		brts	pwm_blinking_h
		sbr	flagsA, 1<<PWM_STATE

pwm_set_fets_h:	sbrs	flagsA, PWM_SYNCHRO
		rjmp	pwm_set_fets_h2
		
		sbrc	flagsA, PWM_S
		SH_off					; If R FET does the PWM
		sbrc	flagsA, PWM_R
		RH_off					; If S FET does the PWM
		sbrc	flagsA, PWM_T
		TH_off

		.if (_PWM_DEAD_CYCLES-6) >= 0
			.if (_PWM_DEAD_CYCLES-6) & 1
				nop
			.endif
			.if (_PWM_DEAD_CYCLES-6) & 2
				rjmp	.+0
			.endif
			.if (_PWM_DEAD_CYCLES-6) & 4
				rjmp	.+0
				rjmp	.+0
			.endif
			.if (_PWM_DEAD_CYCLES-6) & 8
				rcall	dtd_ret
				nop
			.endif
			.if (_PWM_DEAD_CYCLES-6) & 16
				rcall	dtd_ret
				rcall	dtd_ret
				rjmp	.+0
			.endif
		.endif

pwm_set_fets_h2:
		sbrc	flagsA, PWM_S
		SL_on					; If R FET does the PWM
		sbrc	flagsA, PWM_R
		RL_on					; If S FET does the PWM
		sbrc	flagsA, PWM_T
		TL_on					; If T FET does the PWM

pwm_ret:	out	_SFR_IO_ADDR(SREG), isreg
		reti

		; PWM blinking mode. If the time between low and high PWM states is not long enough to execute
		; two interrupts, we do just one interrupt, which switches one state, waits in a loop and switches another state.
		; The point is to reduce power "bumps" around 0% throttle and 100% throttle.
		; Well, noone in fact cares about power bump around 0% throttle, it's just implemented for the sake of formality.

pwm_blinking_h:	mov	tmp_l, pwm_low_l		; Copy the blink time to tmp_l
		lds	tmp_h, CONST_3			; tmp_h = 3. Can't really use any r16+ register here.

		bst	flagsA, PWM_R			; If R FET does the PWM
		brts	pwm_blink_h_r
		bst	flagsA, PWM_T			; If T FET does the PWM
		brts	pwm_blink_h_t

pwm_blink_h_s:	SL_on					; If S FET does the PWM
		sub	tmp_l, tmp_h
		brpl	.-4
		SL_off
		out	_SFR_IO_ADDR(SREG), isreg
		reti

pwm_blink_h_r:	RL_on
		sub	tmp_l, tmp_h
		brpl	.-4
		RL_off
		out	_SFR_IO_ADDR(SREG), isreg
		reti

pwm_blink_h_t:	TL_on
		sub	tmp_l, tmp_h
		brpl	.-4
		TL_off
		out	_SFR_IO_ADDR(SREG), isreg
		reti



		; If it was high pwm state, and we will be doing low state now.
pwm_set_low:	in	tmp_l, _SFR_IO_ADDR(OCR2)	; Calculate and set time of the next high state
		add	tmp_l, pwm_low_l
		out	_SFR_IO_ADDR(OCR2), tmp_l
		mov	pwm_tcnt2_h, pwm_low_h

		; Description in complemenatry code above.
		tst	pwm_low_l
		;in	tmp_l, _SFR_IO_ADDR(OCR2)
		in	tmp_h, _SFR_IO_ADDR(TCNT2)
		brmi	pwm_tch_done
		sub	tmp_l, tmp_h
		brpl	pwm_tch_done

		clr	tmp_l
		set
		bld	tmp_l, OCF2
		out	_SFR_IO_ADDR(TIFR), tmp_l
		dec	pwm_tcnt2_h

pwm_tch_done:	bst	flagsA, PWM_BLINKING
		brts	pwm_blinking_l

		cbr	flagsA, 1<<PWM_STATE

pwm_set_fets_l:	sbrc	flagsA, PWM_S
		SL_off
		sbrc	flagsA, PWM_R
		RL_off
		sbrc	flagsA, PWM_T
		TL_off
		
		wdr
		out	_SFR_IO_ADDR(SREG), isreg

		// Check if synchronous PWM is set.
		sbrs	flagsA, PWM_SYNCHRO
		reti

		// It is, it's already been 10 cycles. See if we need to add some more.
		.if (_PWM_DEAD_CYCLES-10) >= 0
			.if (_PWM_DEAD_CYCLES-10) & 1
				nop
			.endif
			.if (_PWM_DEAD_CYCLES-10) & 2
				rjmp	.+0
			.endif
			.if (_PWM_DEAD_CYCLES-10) & 4
				rjmp	.+0
				rjmp	.+0
			.endif
			.if (_PWM_DEAD_CYCLES-10) & 8
				rcall	dtd_ret
				nop
			.endif
			.if (_PWM_DEAD_CYCLES-10) & 16
				rcall	dtd_ret
				rcall	dtd_ret
				rjmp	.+0
			.endif
		.endif
		sbrc	flagsA, PWM_S
		SH_on					; If R FET does the PWM
		sbrc	flagsA, PWM_R
		RH_on					; If S FET does the PWM
		sbrc	flagsA, PWM_T
		TH_on
		reti

		; Low-state-blinking PWM mode. More detailed description in complemenatry code above.
pwm_blinking_l:	mov	tmp_l, pwm_high_l
		lds	tmp_h, CONST_3

		bst	flagsA, PWM_R
		brts	pwm_blink_l_r
		bst	flagsA, PWM_T
		brts	pwm_blink_l_t

pwm_blink_l_s:	SL_off
		sub	tmp_l, tmp_h
		brpl	.-4
		SL_on
		out	_SFR_IO_ADDR(SREG), isreg
		reti

pwm_blink_l_r:	RL_off
		sub	tmp_l, tmp_h
		brpl	.-4
		RL_on
		out	_SFR_IO_ADDR(SREG), isreg
		reti

pwm_blink_l_t:	TL_off
		sub	tmp_l, tmp_h
		brpl	.-4
		TL_on
		out	_SFR_IO_ADDR(SREG), isreg
		reti