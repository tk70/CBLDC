/*
 * bldc.h
 *
 * Created: 2014-12-18 19:40:10
 *  Author: Jakub Turowski
 *
 * ESC settings, board independent.

 * Ones marked with [DEFAULT] will be used unless different settings are programmed by user.
 */ 


#ifndef BLDC_H_
#define BLDC_H_

// *------------------*
// |    PROGRAMMING   |
// *------------------*
// Settings for stick programming
#define PROG_TIMNIG_LOW 9
#define PROG_TIMNIG_MID 14
#define PROG_TIMNIG_HIGH 19

#define PROG_PWM_FREQ_1 8000
#define PROG_PWM_FREQ_2 16000
#define PROG_PWM_FREQ_3 20000

#define PROG_REPEAT_NUM 3

// *------------------*
// |      General     |
// *------------------*
// Location of the board-specific config file
#define BOARD "boards/n11e2.h"

// Timing advance angle
#define TIMING_ADVANCE PROG_TIMNIG_MID	// [°]

// Percent of throttle allowed per 1000 RPM.
// The point of it is to limit current at low speeds.
#define THROT_PER_KRPM 15				// [%]

#define THOTTLE_SPEED 14

// Rotation direction (0/1)
#define ROTATION_DIRECTION 0

// Software programmed limit of motor speed, the ESC will limit energy above it, not to exceed it.
#define RPM_MAX 300000					// [RPM]

// Blind angle from commutation to ZC scan.
#define BLIND_ANGLE 5					// [°]

// [DEFAULT] Brake enabled 
// 0/1
#define BRAKE_ENABLED 0

// Regenerative brake.
// 1 - energy from braking will be returned to the power source
// 0 - energy from braking will be burnt into heat in the motor and ESC.
#define BRAKE_REGENERATIVE 1

// [DEFAULT] output PWM frequency
#define PWM_FREQUENCY PROG_PWM_FREQ_2	// [Hz]

// [DEFAULT] Use synchronous PWM
#define PWM_SYNCHRONOUS 0

// Delay between turning one FET off and the other on in one bridge.
// For Synchronous PWM only.
#define PWM_DEAD_TIME 600				// [ns]



// *------------------*
// |      Signal      |
// *------------------*

// *------------------*
// |      RC PWM      |
// *------------------*

// [DEFAULT]
#define RC_PWM_MIN 800					// [us]

// [DEFAULT]
#define RC_PWM_LOW 1050					// [us]

// [DEFAULT]
#define RC_PWM_HIGH 1850				// [us]

// [DEFAULT]
#define RC_PWM_MAX 2200					// [us]

// 
#define RC_PWM_BRAKE_THRESHOLD 60		// [us]
#define RC_PWM_TIMEOUT 100				// [cs] (centiseconds! 1cs = 10ms)


// *------------------*
// |     Start-up     |
// *------------------*
#define START_MIN_POWER 10				// [%]
#define START_MAX_POWER 16				// [%]
#define START_MAX_FORCED_RPM 400
#define START_MIN_FORCED_RPM 180
#define START_STEP_US 100				// [us]
#define START_ATTEMPTS 4


// *------------------*
// |     GOVERNOR     |
// *------------------*

// [DEFAULT] Governor mode enabled?
#define GOVERNOR_ENABLED 0

// [DEFAULT] Governor max speed at full power signal
#define GOV_MAX_SPEED 60000				// [RPM]

// The controller will try not to go below that speed, on non-zero signal.
#define GOV_MIN_SPEED 8000				// [RPM]

// Don't touch these unless you know well what you're doing!
#define GOV_SAMPLING_FREQ 500			// [Hz]
//#define GOV_ANTIWINDUP 100				// [%]
#define GOV_THROTTLE_SPEED 500			// [%/s]

// PID controller settings (independent notation)
// U(s) = E(s) * (P + 1/(Ti*s))
// Proportional gain
#define GOV_P 3	

// Integration time	
// 0xFFFF to disable integration					
#define GOV_Ti 250					// [ms]

#define GOV_D 8

#include BOARD

#endif /* BLDC_H_ */