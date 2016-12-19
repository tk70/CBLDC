/*
 * cbldc.c
 *
 * Created: 2014-12-18 19:24:05
 *  Author: Jakub Turowski
 */ 

#include <string.h>				// memcpy
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "bldc.h"
#include "power_stage.h"
#include "comparator.h"
#include "led.h"
#include "timer.h"
#include "globals.h"
#include "pwm.h"
#include "signal.h"
#include "speed.h"
#include "beep.h"
#include "commutation.h"
#include "governor.h"
#include "config.h"
#include <util/delay.h>

#if (TIMING_ADVANCE > 30) || (TIMING_ADVANCE < 0)
	#error Invalid constant: TIMING_ADVANCE. 0-30 allowed.
#endif

typedef struct {
	uint16_t com_duration;
	uint16_t previous_zc_time;
	uint16_t predicted_zc_time;
} timing;

uint8_t check_signal(uint8_t state)
{
	uint8_t cnt = 4;
	while (cnt) {
		wdt_reset();
		signal_process();
		if (signal_error()) return 0;
		if (flag_is_set(flagsB, SIGNAL_RECEIVED)) {
			clear_flag(flagsB, SIGNAL_RECEIVED);
			if (state) {
				if (signal_max()) {
					cnt--;
				}
				else return 0;
			}
			else {
				if (signal_get_power() == 0) {
					cnt--;
				}
				else return 0;
			}
			if (cnt == 0) return 1;
		}
	}	
}

// *-------------------------------------------*
// |            Stick programming              |
// *-------------------------------------------*

#define PROG_TIMEOUT 0
#define PROG_SELECTED 1

void program_delay()
{
	beep_sound(0, 50);
}

static void program_beep_n(uint8_t n, uint16_t period, uint8_t time)
{
	while (n--) {
		beep_sound(period, time);
		beep_sound(0, 10);
	}
}

static uint8_t program_wait_choice(uint8_t state, uint8_t low_sounds, uint8_t high_sounds)
{
	uint8_t i = PROG_REPEAT_NUM;
	do {
		program_beep_n(low_sounds,  F_TO_T(BEEP_C5), 30);
		program_beep_n(high_sounds, F_TO_T(BEEP_G6), 10);
		timerAX_set(timerAX_get() + TICKS_PER_SECOND);
		while (!timerAX_ready()) {
			signal_process();
			if (check_signal(state)) {
				return PROG_SELECTED;
			}
			wdt_reset();
		}
	} while (--i);
	return PROG_TIMEOUT;
}

static uint8_t program_submenu(uint8_t positions)
{
	int8_t i;
	while (1) {
		for (i = 1; i <= positions; i++) {
			if (program_wait_choice(1, 0, i) == PROG_SELECTED) {
				return i;
			}
		}
	}
}

static uint8_t program_menu_select(uint8_t low_sounds, uint8_t high_sounds, uint8_t submenu_positions)
{
	if (program_wait_choice(0, low_sounds, high_sounds) == PROG_SELECTED) {
		beep_play(&beep_prog_submenu);
		program_delay();
		return program_submenu(submenu_positions);
	}
	return 0;
}

static void program_store(config* cfg, uint8_t sounds)
{
	config_write(cfg, &_cfg_user);
	program_beep_n(sounds, F_TO_T(BEEP_A6), 10);
	program_delay();
	beep_play(&beep_prog_success);
}

static void program_menu(config* cfg)
{
	uint8_t s;
	
	// Brake
	// beep
	s = program_menu_select(0, 1, 2);
	if (s) {
		switch (s) {
			case 1: CBI(cfg->flags, CFG_BRAKE); break;
			default: SBI(cfg->flags, CFG_BRAKE); break;
		}
		program_store(cfg, s);
	}
	
	// Timing
	// beep-beep
	s = program_menu_select(0, 2, 3);
	if (s) {
		switch (s) {
			case 1: cfg->timing_delay = (30-PROG_TIMNIG_LOW)*128/30; break;
			case 2: cfg->timing_delay = (30-PROG_TIMNIG_MID)*128/30; break;
			default: cfg->timing_delay = (30-PROG_TIMNIG_HIGH)*128/30; break;
		}
		program_store(cfg, s);
	}
	
	// Governor
	// beep-beep-beep
	s = program_menu_select(0, 3, 2);
	if (s) {
		switch (s) {
			case 1: CBI(cfg->flags, CFG_GOVERNOR); break;
			default: SBI(cfg->flags, CFG_GOVERNOR); break;
		}
		program_store(cfg, s);
	}
	
	// Rotation direction
	// beep-beep-beep-beep
	s = program_menu_select(0, 4, 2);
	if (s) {
		switch (s) {
			case 1: CBI(cfg->flags, CFG_DIRECTION); break;
			default: SBI(cfg->flags, CFG_DIRECTION); break;
		}
		program_store(cfg, s);
	}
	
	// PWM frequency and mode
	// BEEP-beep
	s = program_menu_select(1, 1, 5);
	if (s) {
		switch (s) {
			case 1: cfg->pwm_freq = PROG_PWM_FREQ_1; break;
			case 2: cfg->pwm_freq = PROG_PWM_FREQ_2; break;
			case 3: cfg->pwm_freq = PROG_PWM_FREQ_3; break;
			case 4: CBI(cfg->flags, CFG_SYNCHRO_PWM); break;
			default: SBI(cfg->flags, CFG_SYNCHRO_PWM); break;
		}
		program_store(cfg, s);
	}
	
	// Reset to default settings
	// BEEEEP-BEEEEP
	s = program_menu_select(2, 0, 2);
	if (s) {
		switch (s) {
			case 1: break;
			default: config_load_default(cfg);
		}
		program_store(cfg, s);
	}
}

static void program(config* cfg)
{
	if (program_wait_choice(1, 0, 0) == PROG_SELECTED) {
		if (program_wait_choice(0, 2, 0) == PROG_TIMEOUT) {
			while (1) {
				program_menu(cfg);
			}
		}
	}
}

// *-------------------------------------------*
// |                   Start                   |
// *-------------------------------------------*

typedef struct {
	uint32_t forced_com_time_max;
	uint32_t forced_com_time_min;
	uint16_t time_step;
	uint8_t min_ok;
	uint8_t max_fail;
} start_config;

const start_config sc_default = {
	RPM_TO_COM_TIME(START_MIN_FORCED_RPM),
	RPM_TO_COM_TIME(START_MAX_FORCED_RPM),
	US_TO_TICKS(START_STEP_US),
	71,
	50
};

#define STARTUP_OK 0
#define STARTUP_NOSIG 1
#define STARTUP_FAIL 2

// Start-up ZC filtering constants. The lower PWM frequency is, the higher they should be.
// Set experimentally.
#define ZC_NUM_OK_PRE 1
#define ZC_NUM_OK 24

static void start_set_power(uint16_t power)
{
	if (power > 0) {
		if (power < pwm_start_min) {
			power = pwm_start_min;		
		}			
		else
		if (power >= pwm_start_max) {
			power = pwm_start_max;
		}		
	}
	pwm_set(power);
	clear_flag(flagsB, SIGNAL_RECEIVED);
}

static uint8_t start_wait_aco(uint8_t state, uint8_t filter_cnt)
{
	uint8_t cnt = 0;
	while (!timerAX_ready()) {
		wdt_reset();
		if (acomp_state() == state) {
			if (++cnt >= filter_cnt) return 1;
		}
		else if (cnt) cnt--;
	}	
	return 0;
}

static uint8_t start_wait_for_zc(uint32_t timeout)
{
	timerAX_set(timeout);
	if (BIS(ACSR, ACIS0)) {
		start_wait_aco(1, ZC_NUM_OK_PRE);
		return start_wait_aco(0, ZC_NUM_OK);
	}
	else {
		start_wait_aco(0, ZC_NUM_OK_PRE);
		return start_wait_aco(1, ZC_NUM_OK);
	}
}

static uint8_t __attribute__((optimize("s"))) start(const start_config* sc, timing* t)
{
	uint32_t previous_zc_time = timerAX_get();
	uint32_t forced_com_time = sc->forced_com_time_max;
	int8_t min_ok = sc->min_ok;
	int8_t max_fail = sc->max_fail;
	int8_t cnt = 6;
	while (1) {
		commutate();
		wdt_reset();
		signal_process();
		uint16_t power = signal_get_power();
		if (power == 0) {
			pwm_set(0);
			return STARTUP_NOSIG;
		}
		// Do not turn the power on in first commutations
		// The motor may be rotating already for some reason, let's just synchronize with it then.
		if (cnt >= 0) {
			cnt --;
		} else {
			start_set_power(power);
		}
		start_set_power(power);
		uint8_t zc = start_wait_for_zc(previous_zc_time + forced_com_time);
		uint32_t zc_time = timerAX_get();	
		uint16_t delta = (uint16_t)zc_time - (uint16_t)previous_zc_time;
		previous_zc_time = zc_time;
		//LED1_x;
		if (zc) {
			if (--min_ok < 0) {
				t->previous_zc_time = previous_zc_time;
				t->predicted_zc_time = previous_zc_time + delta;
				t->com_duration = delta;
				return STARTUP_OK;
			}
		}
		else {
			if (--max_fail == 0) {
				pwm_set(0);
				return STARTUP_FAIL;
			}
		}
		forced_com_time -= sc->time_step;
		if (forced_com_time < sc->forced_com_time_min) {
			forced_com_time = sc->forced_com_time_min;
		}
		#if BLIND_ANGLE
		if (zc) {
			uint16_t zc_scan_start = (uint16_t)zc_time + mul_16_frac8(delta, BLIND_ANGLE*128/30);
			_noinline_timerA_wait_until(zc_scan_start);
		}		
		#endif
	}
}


// *-------------------------------------------*
// |                    Run                    |
// *-------------------------------------------*

#define RUN_TIMEOUT 0
#define RUN_BRAKE 1

static uint16_t run_calculate_power(uint16_t speed)
{
	uint16_t limit = mul_16_8_sum_frac8_sat16(speed, sttl_mul, sttl_frac);
	uint16_t throt_inertia = pwm_get()+THOTTLE_SPEED;
	if (limit >= throt_inertia) limit = throt_inertia;
	if (limit >= pwm_range) limit = pwm_range;
	if (speed >= RPM_MAX/60) limit >>= 1;
	uint16_t power;
	if (flag_is_set(flagsB, GOVERNOR)) {
		power = governor_get_power();
	}
	else {
		power = signal_get_power();
	}
	if (power >= limit) power = limit;
	return power;
}

uint8_t __attribute__((optimize("2"))) run(const timing* t)
{	
	// The static ones will be stored in RAM.
	static int8_t calculation_step;
	static int8_t zc_timeout;
	static uint16_t power;
	static uint16_t rps;
	uint16_t com_duration = t->com_duration;
	uint16_t previous_zc_time = t->previous_zc_time;
	rps = com_time_to_rps(com_duration);
	calculation_step = 0;
	zc_timeout = 0;
	commutate();
	zc_run_begin(); // <- opt
	governor_begin(pwm_get());
	timerA_set(t->predicted_zc_time+com_duration);
	//LED0_0;
	while (1) {
		uint16_t zc_time;
		
		
		/*
		In this loop we will be waiting for Zero-Cross signal from the comparator which is working in interrupt driven
		auto mode, which means it doesn't need to be scanned manually.
		In the meantime, we will be handling operations that need to be done, like signal processing and power control.
		
		Hack: go at least once into case 2 (power calculation).
		RPS might have changed, new power limit may need to be calculated.
		*/
		set_flag(flagsB, SIGNAL_RECEIVED);
		while (1) {
			switch (++calculation_step) {
				case 1:
					signal_process();
					if (flag_is_set(flagsB, SIGNAL_RECEIVED)) {
						clear_flag(flagsB, SIGNAL_RECEIVED);
						if (signal_brake()) return RUN_BRAKE;					
					}
					else {
						calculation_step = 4;
					}
					break;
					
				case 2:
					power = run_calculate_power(rps);
					break;
					
				case 3:
					if (zc_timeout != 2) pwm_set(power);
					break;
					
				/* Governor calculations. They are quite long all together, so I split them into 3 parts. */
				case 4:
					if (governor_needs_process()) {
						governor_process_feedback(rps);
					}
					else {
						calculation_step = 0;
					}
					break;
				case 5:
					governor_process_error(signal_get_power());
					break;	
				default:
					governor_process_pid();
					calculation_step = 0;
					break;
			}
			
			/* If comparator has detected ZC*/
			if (zc_run_detected()) {
				zc_time = zc_run_time();
				break;
			}			
			
			/* If ZC timeout occurred*/
			if (timerA_ready()) {
				
				/* Check if we are still waiting for PRE-ZC state on the comparator.*/
				if (flag_is_set(flagsB, AWAIT_PRE_ZC)) {
					
					/* Yes, we are. 60° have passed and there's still no ZC-preceding state.
					It sometimes happens if the throttle was opened too quick, and excessive current is flowing
					through the motor. Set power to zero, set ZC detection time to estimated ZC time, and continue
					as if nothing happened. */
					//LED0_1;
					pwm_set(0);
					zc_timeout = 2;
					zc_time = previous_zc_time + com_duration;
					break;
				}
				
				else {
					/* No, we are waiting for actual ZC edge now */
					if (!zc_timeout) {
						/* If it's just the 60° timeout, let's wait 180 more degrees */
						zc_timeout = 1;
						timerA_set(previous_zc_time + com_duration * 4);
					}
					/* If 4 commutation lengths have passed since the previous ZC, the motor must have stopped. Return. */
					else return RUN_TIMEOUT;				
				}
			}
		}
		
		zc_timeout = 0;
		
		/* ZC has been detected.
		Filter ZC time, using simple IIR. */
		uint16_t delta = zc_time - previous_zc_time;
		com_duration += delta;
		com_duration /= 2;
		zc_time = previous_zc_time + com_duration;
		uint16_t timing_interval = mul_16_frac8(com_duration, cfg.timing_delay);
		timerA_set(zc_time + timing_interval);
		previous_zc_time = zc_time;
		uint16_t next_zc_timeout = zc_time + com_duration;
		rps = com_time_to_rps(com_duration);		
		
		// Wait until it's time to commutate
		timerA_wait_ready();
		commutate();
		wdt_reset();
		
		#if BLIND_ANGLE
		uint16_t zc_scan_start = zc_time + mul_16_frac8(com_duration, BLIND_ANGLE*128/30);
		timerA_wait_until(zc_scan_start);
		#endif
		
		// Begin background ZC scan
		zc_run_begin();
		
		// Set the ZC detection timeout, as this ZC time + commutation length.
		timerA_set(next_zc_timeout);
	}
}

// *-------------------------------------------*
// |                   Brake                   |
// *-------------------------------------------*

static void brake_motor()
{
	cli();
	RH_off();
	SH_off();
	TH_off();
	uint16_t time = timer_get();	// timer_get does sei()
	uint16_t on_duty = 0;
	while (1) {
		RL_on();
		SL_on();
		TL_on();
		//LED0_1;
		_noinline_timerA_wait_until(time+on_duty);
		
		// Leaving one FET open makes current flow through it, and thus the energy goes into heat in the FET and motor winding.
		// Leaving all FETS closed makes the winding inductance discharge into the power source through the body diodes.
		#if BRAKE_REGENERATIVE
			RL_off();
		#endif
		SL_off();
		TL_off();
		time += US_TO_TICKS(500);
		signal_process();
		_noinline_timerA_wait_until(time);
		wdt_reset();
		on_duty += 1;
		if (!signal_brake() || signal_error()) break;
		if (on_duty >= US_TO_TICKS(450)) {
			on_duty = US_TO_TICKS(450);
			
		}				
	}
	#if !BRAKE_REGENERATIVE
		RL_off();
	#endif
}

static void brake_init()
{
	if (BIS(cfg.flags, CFG_BRAKE)) {
		set_flag(flagsB, BRAKE);
	}
}

static uint8_t __attribute__((optimize("s"))) run_motor()
{
	timing t;
	uint8_t n = START_ATTEMPTS;
	//uint8_t n = 1;
	while (n--) {
		switch (start(&sc_default, &t)) {
			case STARTUP_OK:
				run(&t);
				pwm_set(0);
				return 0;
			case STARTUP_NOSIG:
				pwm_set(0);
				return 0;
		}
	}
	pwm_set(0);
	return 1;
}

static void __attribute__((optimize("s"))) loop()
{
	uint8_t enabled = 0;
	while (1) {
		if (!enabled) while (!check_signal(0));

		// Beep signal connect and disconnect
		if (signal_error()) {
			if (enabled) {
				beep_play(&beep_lost_signal);
				enabled = 0;
			}
		} else {
			if (!enabled) {
				beep_play(&beep_got_signal);
				enabled = 1;
			}
		}
		wdt_reset();
		signal_process();
		
		if (signal_get_power() > 0) {
			if (run_motor() != 0) {
				//beep_play(&beep_start_fail);
				//enabled = 0;
			}
		}		
		
		if (signal_brake()) {
			brake_motor();
		}
	}
}

int __attribute__((optimize("s"))) main(void)
{
	power_stage_init();
	led_init();
	timer_init();
	
	program_delay();
	
	// Load config from EEPROM, into the
	if (config_load(&cfg, &_cfg_user)) {
		// If loading config from EEPROM failed.
		LED0_1;
	}
	
	config* cp =  &cfg;
	
	calculate_globals();
	signal_init();
	pwm_init();
	commutation_init();
	
	// Go to ESC programming
	program(cp);
	
	// Now when we got configuration loaded, initialize other stuff.
	acomp_init();
	governor_init();
	brake_init();
	wdt_enable(WDTO_30MS);
	LED0_0;
	loop();
}
