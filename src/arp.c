/*   arp.c - Address Resolution Protocol

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/
     
#include "ether.h"
#include "arp.h"         
#include "sock.h"
#include "sys.h"
#include "dhcp.h"



//arp_cache lo_arp_cache[CACHE_SIZE];


//------------------------------------------------------------------------
// arp_send:
// Builds a outgoing ARP packet
//------------------------------------------------------------------------
void arp_send(const uint8_t *hw_addr, uint32_t ip_addr, uint16_t op)
{
     // Put all needed information to the ARP datastructure
     arp_ptr_tx_hw_type = ARP_TYPE_ETH;
     arp_ptr_tx_prot_type = ARP_TYPE_IP;
     arp_ptr_tx_hw_len = 6;
     arp_ptr_tx_prot_len = 4;
     arp_ptr_tx_op = op;
     memcpy(arp_ptr_tx_hw_src, lo_hw_addr, 6);
     arp_ptr_tx_prot_src = lo_ip_addr;
     memcpy(arp_ptr_tx_hw_dst, hw_addr, 6);    
     arp_ptr_tx_prot_dst = ip_addr;

     // Convert from Little Endian to Big Endian
     arp_ptr_tx_hw_type = memswap(arp_ptr_tx_hw_type);
     arp_ptr_tx_prot_type = memswap(arp_ptr_tx_prot_type);
     arp_ptr_tx_op = memswap(arp_ptr_tx_op);
     arp_ptr_tx_prot_src = memswap32(arp_ptr_tx_prot_src);  
     arp_ptr_tx_prot_dst = memswap32(arp_ptr_tx_prot_dst);   

     // Send the package to ethernet     
     ether_send(hw_addr, ETHER_TYPE_ARP, ARP_PKT_LEN);  
}



//------------------------------------------------------------------------
// arp_recv:
// Deals with incomming ARP packages, if they are ARP request or
// ARP replies
//------------------------------------------------------------------------
void arp_recv(arp_packet *arp_ptr)
{
     // Checks if it is possible to receive a ARP packet
     if(dhcp_check() == FALSE)
     {
          return;
     }
 
     // Convert Big Endian to Little Endian
     arp_ptr->hw_type = memswap(arp_ptr->hw_type);
     arp_ptr->prot_type = memswap(arp_ptr->prot_type);
     arp_ptr->op = memswap(arp_ptr->op);
     arp_ptr->prot_src = memswap32(arp_ptr->prot_src);  
     arp_ptr->prot_dst = memswap32(arp_ptr->prot_dst);

     // Validate incoming ARP packet, so everything is correct
     if((arp_ptr->hw_type != ARP_TYPE_ETH) || (arp_ptr->prot_type != ARP_TYPE_IP) || 
        (arp_ptr->hw_len != 6) || (arp_ptr->prot_len != 4) || (arp_ptr->prot_dst != lo_ip_addr))
     {
          return;
     }
    
     // Checks what shall be done with the packet
     switch(arp_ptr->op)
     {
          case ARP_OP_REQUEST:
               // Send a ARP reply
               arp_send(arp_ptr->hw_src, arp_ptr->prot_src, ARP_OP_REPLY);
               break;
    
          case ARP_OP_REPLY:
               // Received a ARP Reply           
               if(arp_ptr->prot_src == gw_ip_addr)
               {
                    memcpy(gw_hw_addr, arp_ptr->hw_src, 6);
                    sock_set_gw(TRUE);
               }
               
               break;    
     }
}     



//------------------------------------------------------------------------
// arp_cache_add:
// ** This is not finished. **
// Our ARP cache works as a FIFO.. it should be better if LRU algo. was
// implemented, but I had no spare time to make that work.
//------------------------------------------------------------------------
//void arp_cache_add(uint8_t *hw_addr, uint8_t *prot_addr)
//{
//     if(arp_cache_ptr < ARP_CACHE_SIZE)
//     {
//          memcpy(lo_arp_cache[arp_cache_ptr].hw_addr, hw_addr, 6);
//          memcpy(lo_arp_cache[arp_cache_ptr].prot_addr, prot_addr, 4);
//          arp_cache_ptr = arp_cache_ptr + 1;
//     }
//     else
//     {
//          arp_cache_ptr = 0;
//          memcpy(lo_arp_cache[arp_cache_ptr].hw_addr, hw_addr, 6);
//          memcpy(lo_arp_cache[arp_cache_ptr].prot_addr, prot_addr, 4);
//          arp_cache_ptr = arp_cache_ptr + 1;
//     }
//}



//------------------------------------------------------------------------
// arp_cache_check:
//------------------------------------------------------------------------
//uint8_t arp_cache_check(uint8_t *hw_addr)
//{
//     uint16_t i;
//
//     for(i = 0; i < ARP_CACHE_SIZE; i++)
//     {
//          if(strcmp(hw_addr, lo_arp_cache[i].hw_addr))
//          {
//               return lo_arp_cache[i].prot_addr;
//          }
//     }
//
//     return NULL;
//}