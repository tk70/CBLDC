/*
 * brs.h
 *
 * Created: 2014-05-13 13:07:28
 *  Author: Jakub Turowski
 *
 * Bit operations
 */ 


#ifndef BRS_H_
#define BRS_H_

#define SBI(var, bit) (var |= (1<<bit))
#define CBI(var, bit) (var &= ~(1<<bit))
#define XBI(var, bit) (var ^= (1<<bit))
#define BIS(var, bit) (var & (1<<bit))
#define BIC(var, bit) (!(var & (1<<bit)))
#define NB(n) (1<<n)

#endif /* BRS_H_ */