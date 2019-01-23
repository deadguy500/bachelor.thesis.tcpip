/*   sock.h - Sockets

     Högskolan Dalarna / Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/



#ifndef SOCK_H_
#define SOCK_H_



#include "sys.h"



// Local configuration for Dalarna University
// IP:    130.243.35.196
// MAC:   00-90-27-93-E8-65
// Mask:  255.255.254.0
// DHCP:  130.243.35.5
// DNS:130.243.35.5
extern uint8_t      bc_hw_addr[6];      // Broadcast Hardware Address (FF-FF-FF-FF-FF-FF)
extern uint32_t     bc_ip_addr;         // Broadcast IP Address (255.255.255.255)
extern uint8_t      lo_hw_addr[6];      // Local hardware Address
extern uint32_t     lo_ip_addr;         // Local IP Address
extern uint8_t      gw_hw_addr[6];      // Gateway Hardware Address
extern uint32_t     gw_ip_addr;         // Gateway IP Address
extern uint32_t     sn_mask;            // Subnet Mask



void sock_init(void);
void sock_listen(uint16_t cur_sock, uint16_t port);
uint16_t sock_read(uint16_t cur_sock, void *data, uint16_t len);
void sock_write(uint16_t cur_sock, const void *data, uint16_t len);
bool sock_try_lock(void);
void sock_set_lock(bool lock_state);
void sock_set_gw(bool gw_state);
bool sock_check_gw(void);

 
  
#endif /* SOCK_H_ */
