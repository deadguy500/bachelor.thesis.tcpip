/*   sys.c - System dependable functions and macros

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se

     Note: Some of the function declared in here are written by a
           former student, Mikael Swartling.
*/



#include "sys.h"
#include "timer.h"



//------------------------------------------------------------------------
// memcpy:
// This function copies data from one place in the memory to another.
//------------------------------------------------------------------------
void memcpy(void *dest, const void *src, uint16_t len)
{
     while(len != 0)
     {
          *((uint8_t *)dest)++ = *((const uint8_t *)src)++;
          len = len - 1;
     }
}



//------------------------------------------------------------------------
// memclr:
// Clear memory, with otherwords but zeros into the bytes.
//------------------------------------------------------------------------
void memclr(void *src, uint16_t len)
{
     while(len != 0)
     {
          *((uint8_t *)src)++ = 0;
          len = len - 1;
     }
}



//------------------------------------------------------------------------
// strcmp:
// Compare two strings.
//------------------------------------------------------------------------
bool strcmp(const uint8_t *str1, const uint8_t *str2)
{
     while((*str1 == *str2) && *str1 && *str2)
     {
          str1++;
          str2++;
     }
  
     return (*str1 == *str2);
}



//------------------------------------------------------------------------
// strlen:
// Returns the length of a data segment.
//------------------------------------------------------------------------
uint16_t strlen(const uint8_t *str)
{   
     int len;

     for(len = 0; *str != '\0'; str++)
     {
          len++;
     }

     return len;
}



//------------------------------------------------------------------------
// checksum:
// Calculates checksums for diffrent protocol. This code is stolen
// from TCP/IP Illustrated, Vol 2 - Gary R. Wright and W. Richard Stevens.
//------------------------------------------------------------------------
uint16_t checksum(const void *data, uint16_t len)
{
     uint32_t sum;

     for(sum = 0; len > 1; len -= 2)
     {
          sum += *((uint16_t *)data)++;
     }

     if(len > 0)
     {
          sum = sum + *((uint8_t *)data);
     }

     while(sum >> 16)
     {
          sum = (sum & 0xffff) + (sum >> 16);
     }

     return ~(uint16_t)sum;
}



//------------------------------------------------------------------------
// memswap:
// To make all communication to work we need functions to convert from
// little endian to network byte order (big endian). This function 
// converts 16 bits datatypes, so called word or short, from Intels
// little endian to big endian.
//------------------------------------------------------------------------
uint16_t memswap(uint16_t src)
{
     uint16_t dest;
  
     ((uint8_t *)&dest)[1] = ((uint8_t *)&src)[0];
     ((uint8_t *)&dest)[0] = ((uint8_t *)&src)[1];

     return dest;
}



//------------------------------------------------------------------------
// memswap32:
// Same as memswap() but this is for datatypes with 32 bits, so called
// doubble words or long.
//------------------------------------------------------------------------
uint32_t memswap32(uint32_t src)
{
     uint32_t dest;
  
     ((uint8_t *)&dest)[3] = ((uint8_t *)&src)[0];
     ((uint8_t *)&dest)[2] = ((uint8_t *)&src)[1];
     ((uint8_t *)&dest)[1] = ((uint8_t *)&src)[2];
     ((uint8_t *)&dest)[0] = ((uint8_t *)&src)[3];
    
     return dest;
}



//------------------------------------------------------------------------
// wait:
// This function is relic from Mikael Swartling. This is used in the
// disp module. Its just make a sleep in some 'time'.,
//------------------------------------------------------------------------
void wait(uint16_t time)
{
     uint32_t wait;
     
     wait = ticks + ((time + 20 / 2) / 20);
  
     while(ticks < wait);
}
