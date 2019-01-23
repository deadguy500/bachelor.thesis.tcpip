/*   sys.h - System dependable functions and macros

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se

     Note: Some of the function declared in here are written by a
           former student, Mikael Swartling.
*/



#ifndef SYS_H_
#define SYS_H_



#define NULL        0
#define FALSE       0
#define TRUE        1
#define EOF         0xFFFF


// I like to se what I am dealing with, thats why i declare
// datatypes like this. This is system independable - some
// systems use one byte to declare a short int and such..
typedef unsigned char bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;



/************************************************************************
 * Bit mask macros
 * IS_ALL_SET: True if all bits b in v are set
 * IS_ANY_SET: True if any bit b in b is set
 * IS_NOT_SET: True if none of the bits b in v are set
 * SET:        Set bits b in v to 1
 * Reset:      Set bits b in v to 0
 ************************************************************************/
#define IS_SET            IS_ANY_SET
#define IS_ALL_SET(v, b)  ((v) & (b)) == (b)
#define IS_ANY_SET(v, b)  ((v) & (b)) !=  0
#define IS_NOT_SET(v, b)  ((v) & (b)) ==  0
#define SET(v, b)         ((v) |= (b))
#define RESET(v, b)       ((v) &= ~(b))



/************************************************************************
 * NWBO: NetWork Byte Order - Defines constants in network byte order
 *                            on little endian systems.
 * PTOL: Pointer TO Long    - Convert a pointer to a pointer to long, and
 *                            derefernces it. IP-adresses can in many
 *                            casesbe treated as a long integer.
 ************************************************************************/
#define NWBO(msb, lsb)      0x##lsb##msb
#define PTOL(p0)            *((u_long *)(p0))



void memcpy(void *dest, const void *src, uint16_t len);
void memclr(void *src, uint16_t len);
uint16_t memswap(uint16_t src);
uint32_t memswap32(uint32_t src);
bool strcmp(const uint8_t *str1, const uint8_t *str2);
uint16_t strlen(const uint8_t *str);
uint16_t checksum(const void *data, uint16_t len);
void wait(uint16_t time);



#endif /* SYS_H_ */
