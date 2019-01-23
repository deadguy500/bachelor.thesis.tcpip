/*   ether.h - Ethernet

     Högskolan Dalarna / Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/


#ifndef ETHER_H_
#define ETHER_H_

#include "sys.h"



#define ETHER_HDR_LEN         14
#define ETHER_FRAME_LEN_MIN   60
#define ETHER_FRAME_LEN_MAX   1514
#define ETHER_TYPE_IP         0x0800
#define ETHER_TYPE_ARP        0x0806



typedef struct
{
     // Ethernet header, 14 bytes
     uint8_t dst[6];     // Destination Hardware Address
     uint8_t src[6];     // Source Hardware Address
     uint16_t type;      // Ethernet Type (ARP or IP)
     uint8_t data[1500]; // Max size of data portion
} ether_frame;
                                                                
                                                                 

// This is an idea taken from the book TCP/IP Illustrated vol. 2.
extern ether_frame tx_buf;    // Transmit buffer
extern ether_frame rx_buf;    // Receive buffer



void ether_init(void);
void ether_poll(void);
void ether_send(const uint8_t *hw_adr, uint16_t type, uint16_t len);



#endif /* ETHER_H_ */
