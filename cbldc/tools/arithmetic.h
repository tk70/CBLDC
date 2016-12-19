/*
 * arithmetic.h
 *
 * Created: 2015-01-02 14:06:52
 *  Author: Jakub Turowski
 * 
 * A bit more complex arithmetic operations, hard to achieve through C code.
 * Looks like AVR-GCC doesn't know how to use "mul".
 */ 


#ifndef ARITHMETIC_H_
#define ARITHMETIC_H_

#define __ATTR__ inline

// *------------------*
// |      16 bit      |
// *------------------*

// a = (a + b) / 2. The 17th bit from addition gets shifted into the result in division.
#define add16_div2(a, b)\
asm volatile (\
	"add %A0, %A1  \n\t"\
	"adc %B0, %B1  \n\t"\
	"ror %B0       \n\t"\
	"ror %A0       \n\t"\
: "+r"(a)\
: "r"(b)\
:\
);\

__ATTR__ uint16_t sum_16_16_div2(uint16_t a, uint16_t b)
{
	
	
}

// result = x * f / 256
__ATTR__ uint16_t mul_16_frac8(uint16_t x, uint8_t f)
{
	uint16_t result;
	asm volatile (
	"mul %A1, %2  \n\t"\
	"mov %A0, r1  \n\t"\
	"mul %B1, %2  \n\t"\
	"clr %B0      \n\t"\
	"add %A0, r0  \n\t"\
	"adc %B0, r1  \n\t"\
	
	"clr r1       \n\t"\
	
	: "=&r"(result)						// result
	: "r"(x), "r"(f)					// arguments
	: 									// clobbered regs
	);
	return result;
}

// (signed)result = (signed)x * f / 256
__ATTR__ int16_t mulsu_16_frac8(int16_t x, uint8_t f)
{
	int16_t result;
	asm volatile (
	"mulsu %B1, %2 \n\t"\
	"movw %A0, r0  \n\t"\
	"mul %A1, %2   \n\t"\
	"add %A0, r1   \n\t"\
	"clr r1        \n\t"\
	"adc %B0, r1   \n\t"\
	
	: "=&r"(result)						// result
	: "a"(x), "a"(f)					// arguments (a because "mulsu")
	: 									// clobbered regs
	);
	return result;
}

// result = x*m + x*f/256;
__ATTR__ uint16_t mul_16_8_sum_frac8(uint16_t x, uint8_t m, uint8_t f)
{
	uint16_t result;
	asm volatile (
	// Multiply x * m and store it in "result"
	"mul %A1, %2  \n\t"\
	"movw %A0, r0 \n\t"\
	"mul %B1, %2  \n\t"\
	"add %B0, r0  \n\t"\
	
	// Multiply x * f, skip the LSB, dividing the result by 256, and add to "result"
	"mul %A1, %3  \n\t"\
	"clr r0       \n\t"\
	"add %A0, r1  \n\t"\
	"adc %B0, r0  \n\t"\
	"mul %B1, %3  \n\t"\
	"add %A0, r0  \n\t"\
	"adc %B0, r1  \n\t"\
	
	"clr r1       \n\t"\
	
	: "=&r"(result)						// result
	: "r"(x), "r"(m), "r"(f)			// arguments
	: 									// clobbered regs
	);
	return result;
}

// result = x*m + x*f/256;
// result = result > 0xFFFF? 0xFFFF : result;
__ATTR__ uint16_t mul_16_8_sum_frac8_sat16(uint16_t x, uint8_t m, uint8_t f)
{
	uint16_t result;
	asm volatile (
	// Multiply x * m and store it in "result"
	"mul %A1, %2  \n\t"\
	"movw %A0, r0 \n\t"\
	"mul %B1, %2  \n\t"\
	"add %B0, r0  \n\t"\
	
	// Multiply x * f, skip the LSB, dividing the result by 256, and add to "result"
	"mul %A1, %3  \n\t"\
	"clr r0       \n\t"\
	"add %A0, r1  \n\t"\
	"adc %B0, r0  \n\t"\
	"mul %B1, %3  \n\t"\
	"add %A0, r0  \n\t"\
	"adc %B0, r1  \n\t"\
	"brcc .+4     \n\t"\
	"ser %A0      \n\t"\
	"ser %B0      \n\t"\
	
	"clr r1       \n\t"\
	
	: "=&d"(result)						// result (d because "ser")
	: "r"(x), "r"(m), "r"(f)			// arguments
	: 									// clobbered regs
	);
	return result;
}

/*
// result = x * f / 2^24
// result = result > 0xFFFF? 0xFFFF : result;
__ATTR__ uint16_t mul_32_16frac24_sat16(uint32_t x, uint16_t f)
{
	uint16_t result;
	asm volatile (
	// 1. xA * mA
	"mul %A1, %A2 \n\t"\
	"mov r30, r1  \n\t"\
	// byte #1 (r0) skipped.
	
	// 2. xB * mA
	"mul %B1, %A2 \n\t"\
	"clr r31      \n\t"\
	"add r30, r0  \n\t"\
	"adc r31, r1  \n\t"\
	
	// 3. xA * mB
	"mul %A1, %B2 \n\t"\
	"clr %A0      \n\t"\
	"add r30, r0  \n\t"\
	"adc r31, r1  \n\t"\
	"adc %A0, %A0 \n\t"\
	
	// byte #2 (r30) done.
	// 4. xB * mB
	"mul %B1, %B2 \n\t"\
	"add r31, r0  \n\t"\
	"adc %A0, r1  \n\t"\
	
	// 5. xC * mA
	"mul %C1, %A2 \n\t"\
	"clr %B0      \n\t"\
	"add r31, r0  \n\t"\
	"adc %A0, r1  \n\t"\
	"adc %B0, %B0 \n\t"\
	
	 // byte #3 (r31) done.
	 // 6. xC * mB
	"mul %C1, %B2 \n\t"\
	"add %A0, r0  \n\t"\
	"adc %B0, r1  \n\t"\
	
	 // 7. xD * mA
	"mul %D1, %A2 \n\t"\
	"add %A0, r0  \n\t"\
	"adc %B0, r1  \n\t"\
	"brcs 1f      \n\t"\
	
	 // byte #4 (%A0) done.
	 // 8. xD * mB
	"mul %D1, %B2 \n\t"\
	"tst r1       \n\t"\
	"brne 1f      \n\t"\
	"add %B0, r0  \n\t"\
	"brcc 2f      \n\t"\
	"1: ser %A0   \n\t"\
	"ser %B0      \n\t"\
	
	 // byte #5 (%B0) done.
	"2: clr r1 \n\t"\

	: "=&d"(result)						// result (d because "ser")
	: "r"(x), "r"(f)					// arguments
	: "r30", "r31"						// clobbered regs
	);
	return result;
}
*/

// result = (L*l + R*r)/256
__ATTR__ uint16_t mul16_frac8_sum_mul16_frac8(uint16_t L, uint8_t l, uint16_t R, uint8_t r)
{
	uint16_t result;
	asm volatile (
	// L * l
	"mul %A1, %2 \n\t"\
	"mov r31, r0 \n\t"\
	"mov %A0, r1 \n\t"\
	"clr %B0     \n\t"\
	"mul %B1, %2 \n\t"\
	"add %A0, r0 \n\t"\
	"adc %B0, r1 \n\t"\
		
	// R * r
	"mul %B3, %4 \n\t"\
	"add %A0, r0 \n\t"\
	"adc %B0, r1 \n\t"\
	"mul %A3, %4 \n\t"\
	"add r31, r0 \n\t"\
	"adc %A0, r1 \n\t"\
	"clr r1      \n\t"\
	"adc %B0, r1 \n\t"\
		
	: "=&r"(result)						// result
	: "r"(L), "r"(l), "r"(R), "r"(r)	// arguments
	: "r31"								// clobbered regs
	);
	return result;
}


// *------------------*
// |      24 bit      |
// *------------------*

typedef union {
	struct {
		int8_t l;
		uint8_t h;
		uint8_t x;	
	} l_h_x;
	
	struct {
		uint16_t lh;
		int8_t x;		
	} lh_x;
	
	struct {
		uint8_t l;
		int16_t hx;		
	} l_hx;
} int24_t;

/*

// (signed)result = (signed)x * m
__ATTR__ int24_t mulsu24_16_8(int16_t x, uint16_t m)
{
	int24_t result;
	asm volatile (
	"mul %A2, %3    \n\t"\
	"movw %A0, r0   \n\t"\
	"mulsu %B2, %3  \n\t"\
	"add %B0, r0    \n\t"\
	"mov %1, r1     \n\t"\
	"clr r1         \n\t"\
	"adc %1, r1     \n\t"\

	: "=&r"(result.lh_x.lh), "=&r"(result.lh_x.x)	// result
	: "a"(x), "a"(m)					// arguments (a because "mulsu")
	: 									// clobbered regs
	);
	return result;
}

// result = x + y
// result = result < 0? 0 : result;
// result = result > 0xFFFF? 0xFFFF : result;
__ATTR__ uint16_t adds_24_16_satu16(int24_t x, int16_t y)
{
	asm volatile (
	"clr r30        \n\t"\
	"sbrc %B2, 7    \n\t"\
	"ser r30        \n\t"\
	"add %A0, %A2   \n\t"\
	"adc %B0, %B2   \n\t"\
	"adc %1, r30    \n\t"\
	"breq 1f        \n\t"\
	"ldi %A0, 0     \n\t"\
	"ldi %B0, 0     \n\t"\
	"brmi 1f        \n\t"\
	"ser %A0        \n\t"\
	"ser %B0        \n\t"\
	"1:             \n\t"\
	: "+d"(x.lh_x.lh)  						// result (d because "ser")
	: "r"(x.lh_x.x), "r"(y)					// arguments
	: "r30"								// clobbered regs
	);
	return x.lh_x.lh;
}
	
// a(24) += b(24)	
#define add24(a, b)\
asm volatile (\
	"add %0, %3  \n\t"\
	"adc %1, %4  \n\t"\
	"adc %2, %5  \n\t"\
	: "+r"(a.l_h_x.l), "+r"(a.l_h_x.h), "+r"(a.l_h_x.x)\
	: "r"(b.l_h_x.l), "r"(b.l_h_x.h), "r"(b.l_h_x.x)\
	:\
)

// a(24) += x(s16) * m(u8)
#define add24_mulsu24_16_8(a, x, m)\
asm volatile (\
	"mulsu %B3, %4  \n\t"\
	"add %1, r0     \n\t"\
	"adc %2, r1     \n\t"\
	"mul %A3, %4    \n\t"\
	"add %0, r0     \n\t"\
	"adc %1, r1     \n\t"\
	"clr r1         \n\t"\
	"adc %2, r1     \n\t"\
	: "+r"(a.l_h_x.l), "+r"(a.l_h_x.h), "+r"(a.l_h_x.x)\
	: "a"(x), "a"(m)\
	:\
)

*/

#endif /* ARITHMETIC_H_ */