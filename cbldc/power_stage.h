/*
 * power_stage.h
 *
 * Created: 2014-12-18 21:00:17
 *  Author: Jakub Turowski
 */ 

#ifndef POWER_STAGE_H_
#define POWER_STAGE_H_

#include "tools/brs.h"
#include "bldc.h"

#ifdef __ASSEMBLER__

#if RL_INVERTING
#define RL_off sbi _SFR_IO_ADDR(RL_PORT), RL_PIN
#else
#define RL_off cbi _SFR_IO_ADDR(RL_PORT), RL_PIN
#endif

#if RL_INVERTING
#define RL_on cbi _SFR_IO_ADDR(RL_PORT), RL_PIN
#else
#define RL_on sbi _SFR_IO_ADDR(RL_PORT), RL_PIN
#endif


#if RH_INVERTING
#define RH_off sbi _SFR_IO_ADDR(RH_PORT), RH_PIN
#else
#define RH_off cbi _SFR_IO_ADDR(RH_PORT), RH_PIN
#endif


#if RH_INVERTING
#define RH_on cbi _SFR_IO_ADDR(RH_PORT), RH_PIN
#else
#define RH_on sbi _SFR_IO_ADDR(RH_PORT), RH_PIN
#endif

// ------ S phase ------

#if SL_INVERTING
#define SL_off sbi _SFR_IO_ADDR(SL_PORT), SL_PIN
#else
#define SL_off cbi _SFR_IO_ADDR(SL_PORT), SL_PIN
#endif

#if SL_INVERTING
#define SL_on cbi _SFR_IO_ADDR(SL_PORT), SL_PIN
#else
#define SL_on sbi _SFR_IO_ADDR(SL_PORT), SL_PIN
#endif


#if SH_INVERTING
#define SH_off sbi _SFR_IO_ADDR(SH_PORT), SH_PIN
#else
#define SH_off cbi _SFR_IO_ADDR(SH_PORT), SH_PIN
#endif


#if SH_INVERTING
#define SH_on cbi _SFR_IO_ADDR(SH_PORT), SH_PIN
#else
#define SH_on sbi _SFR_IO_ADDR(SH_PORT), SH_PIN
#endif

// ------ T phase ------

#if TL_INVERTING
#define TL_off sbi _SFR_IO_ADDR(TL_PORT), TL_PIN
#else
#define TL_off cbi _SFR_IO_ADDR(TL_PORT), TL_PIN
#endif

#if TL_INVERTING
#define TL_on cbi _SFR_IO_ADDR(TL_PORT), TL_PIN
#else
#define TL_on sbi _SFR_IO_ADDR(TL_PORT), TL_PIN
#endif


#if TH_INVERTING
#define TH_off sbi _SFR_IO_ADDR(TH_PORT), TH_PIN
#else
#define TH_off cbi _SFR_IO_ADDR(TH_PORT), TH_PIN
#endif


#if TH_INVERTING
#define TH_on cbi _SFR_IO_ADDR(TH_PORT), TH_PIN
#else
#define TH_on sbi _SFR_IO_ADDR(TH_PORT), TH_PIN
#endif
	
#else

// ------ R phase ------

inline void RL_init()
{
	SBI(RL_DDR, RL_PIN);
}

inline void RH_init()
{
	SBI(RH_DDR, RH_PIN);
}

inline void RL_off()
{
	#if RL_INVERTING
	SBI(RL_PORT, RL_PIN);
	#else
	CBI(RL_PORT, RL_PIN);
	#endif
}

inline void RL_on()
{
	#if RL_INVERTING
	CBI(RL_PORT, RL_PIN);
	#else
	SBI(RL_PORT, RL_PIN);
	#endif
}

inline void RH_off()
{
	#if RH_INVERTING
	SBI(RH_PORT, RH_PIN);
	#else
	CBI(RH_PORT, RH_PIN);
	#endif
}

inline void RH_on()
{
	#if RH_INVERTING
	CBI(RH_PORT, RH_PIN);
	#else
	SBI(RH_PORT, RH_PIN);
	#endif
}

// ------ S phase ------

inline void SL_init()
{
	SBI(SL_DDR, SL_PIN);
}

inline void SH_init()
{
	SBI(SH_DDR, SH_PIN);
}

inline void SL_off()
{
	#if SL_INVERTING
	SBI(SL_PORT, SL_PIN);
	#else
	CBI(SL_PORT, SL_PIN);
	#endif
}

inline void SL_on()
{
	#if SL_INVERTING
	CBI(SL_PORT, SL_PIN);
	#else
	SBI(SL_PORT, SL_PIN);
	#endif
}

inline void SH_off()
{
	#if SH_INVERTING
	SBI(SH_PORT, SH_PIN);
	#else
	CBI(SH_PORT, SH_PIN);
	#endif
}

inline void SH_on()
{
	#if SH_INVERTING
	CBI(SH_PORT, SH_PIN);
	#else
	SBI(SH_PORT, SH_PIN);
	#endif
}

// ------ T phase ------

inline void TL_init()
{
	SBI(TL_DDR, TL_PIN);
}

inline void TH_init()
{
	SBI(TH_DDR, TH_PIN);
}

inline void TL_off()
{
	#if TL_INVERTING
	SBI(TL_PORT, TL_PIN);
	#else
	CBI(TL_PORT, TL_PIN);
	#endif
}

inline void TL_on()
{
	#if TL_INVERTING
	CBI(TL_PORT, TL_PIN);
	#else
	SBI(TL_PORT, TL_PIN);
	#endif
}

inline void TH_off()
{
	#if TH_INVERTING
	SBI(TH_PORT, TH_PIN);
	#else
	CBI(TH_PORT, TH_PIN);
	#endif
}

inline void TH_on()
{
	#if TH_INVERTING
	CBI(TH_PORT, TH_PIN);
	#else
	SBI(TH_PORT, TH_PIN);
	#endif
}

inline void power_stage_init()
{
	RH_init();
	RH_off();
	SH_init();
	SH_off();
	TH_init();
	TH_off();
	RL_init();
	RL_off();
	SL_init();
	SL_off();
	TL_init();
	TL_off();
}

#endif /* !ASSEMBLER */

#endif /* POWER_STAGE_H_ */