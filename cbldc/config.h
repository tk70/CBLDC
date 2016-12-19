/*
 * config.h
 *
 * Created: 2015-01-10 13:17:30
 *  Author: Jakub Turowski
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/eeprom.h>
#include "bldc.h"

#define CFG_GOVERNOR 0
#define CFG_DIRECTION 1
#define CFG_BRAKE 2
#define CFG_SYNCHRO_PWM 3


// ESC configuration structure
typedef struct {
	// Must be n*16 bits!
	uint16_t rcp_min;
	uint16_t rcp_low;
	uint16_t rcp_high;
	uint16_t rcp_max;
	uint16_t pwm_freq;
	uint8_t timing_delay;
	uint8_t flags;
	uint16_t gov_max_rps;
	uint16_t checksum;			// for EEPROM storage
} config;

const config _cfg_default = {
	rcp_min:		US_TO_TICKS(RC_PWM_MIN),
	rcp_low:		US_TO_TICKS(RC_PWM_LOW),
	rcp_high:		US_TO_TICKS(RC_PWM_HIGH),
	rcp_max:		US_TO_TICKS(RC_PWM_MAX),
	pwm_freq:		PWM_FREQUENCY,
	timing_delay:	(30-TIMING_ADVANCE)*128/30,
	flags:			 (GOVERNOR_ENABLED<<CFG_GOVERNOR)
					|(ROTATION_DIRECTION<<CFG_DIRECTION)
					|(BRAKE_ENABLED<<CFG_BRAKE)
					|(PWM_SYNCHRONOUS<<CFG_SYNCHRO_PWM),
	gov_max_rps:	GOV_MAX_SPEED / 60,
	checksum:		0
};

config EEMEM _cfg_user;

// Sum all words of config besides ending checksum value
uint16_t config_calculate_checksum(const config* c)
{
	uint16_t checksum = 0;
	uint8_t i;
	for (i = 0; i < (sizeof(config)-sizeof(uint16_t))/2; i++) {
		checksum += ((uint16_t*)c)[i];
	}
	return checksum;
}

// See if config makes sense and it's checksum matches.
// We must be 100% sure since an error in some settings could have some unpleasant consequences.
static uint8_t config_evaluate(const config* c)
{
	uint16_t checksum = config_calculate_checksum(c);
	if (checksum == 0 || checksum != c->checksum) return 1;
	if (c->pwm_freq < 1000 || c->pwm_freq >= 32001) return 1;
	if (c->timing_delay > 128) return 1;
	return 0;
}

static void config_load_default(config* c)
{
	memcpy((void*)c, (void*)&_cfg_default, sizeof(config));
}

static uint8_t config_load(config* c, const config* eep_c)
{
	eeprom_read_block(c, eep_c, sizeof(config));
	if (config_evaluate(c)) {
		config_load_default(c);
		return 1;
	}
	else {
		return 0;
	}
}

// Recalculate the config's checksum and store it in ROM.
static void config_write(config* c, config* eep_c)
{
	LED1_1;
	c->checksum = config_calculate_checksum(c);
	eeprom_update_block(c, eep_c, sizeof(config));
}

#endif /* CONFIG_H_ */