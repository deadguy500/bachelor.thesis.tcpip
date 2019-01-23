/*   ip.c - Internet Protocol

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/

#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"
#include "sock.h" 
#include "sys.h"


// This variable is used for the identification field in
// IP header to identify the fragment.
static uint16_t ident = 0;



//------------------------------------------------------------------------
// ip_send:
// This function is called when the system whats to send an IP datagram.
// It fills the IP header with information, calculate checksum and send
// it to the ethernet layer.
//------------------------------------------------------------------------
void ip_send(const uint8_t *hw_addr, uint32_t ip_addr, uint8_t prot, uint16_t len)
{  
     len = len + IP_HDR_LEN;

     // Fills the datastructure with information
     ip_hdr_tx_ver_hl = IP_VERSION | 5;
     ip_hdr_tx_tos = 0;
     ip_hdr_tx_len = len;
     ip_hdr_tx_ident = ++ident;
     ip_hdr_tx_frag = 0;
     ip_hdr_tx_ttl = 64;
     ip_hdr_tx_prot = prot;
     ip_hdr_tx_csum = 0;
     ip_hdr_tx_src_addr = lo_ip_addr;
     ip_hdr_tx_dst_addr = ip_addr;

     // Change from Little Endian to Big Endian (Network Byte Order)
     ip_hdr_tx_len = memswap(ip_hdr_tx_len);
     ip_hdr_tx_ident = memswap(ip_hdr_tx_ident);
     ip_hdr_tx_frag = memswap(ip_hdr_tx_frag);
     ip_hdr_tx_src_addr = memswap32(ip_hdr_tx_src_addr);
     ip_hdr_tx_dst_addr = memswap32(ip_hdr_tx_dst_addr);
  
     // Calculate checksum
     ip_hdr_tx_csum = checksum(ip_hdr_tx, IP_HDR_LEN);

     // Checks if the remote host is on the same network as
     // the local host. If not, the destination hardware address
     // is set to the hardware address of the gateway.
     if((ip_addr & sn_mask) != (lo_ip_addr & sn_mask))
     {
          hw_addr = gw_hw_addr;           
     }
      
     // Send the IP datagram to the ethernet layer
     ether_send(hw_addr, ETHER_TYPE_IP, len);
}



//------------------------------------------------------------------------
// ip_recv:
// Receives IP datagrams from the Ethernet layer. If the datagram is
// usable this function sends it further to either ICMP, TCP or UDP.
//------------------------------------------------------------------------
void ip_recv(ip_header *ip_hdr_ptr)
{
     uint16_t chsum;
     
     // Calculate checksum
     chsum = checksum(ip_hdr_ptr, ( (ip_hdr_ptr->ver_hl & 0x0F) * 4) );
     
     // Converts from Big Endian to Little Endian
     ip_hdr_ptr->len = memswap(ip_hdr_ptr->len);
     ip_hdr_ptr->ident = memswap(ip_hdr_ptr->ident);
     ip_hdr_ptr->frag = memswap(ip_hdr_ptr->frag);
     ip_hdr_ptr->src_addr = memswap32(ip_hdr_ptr->src_addr);
     ip_hdr_ptr->dst_addr = memswap32(ip_hdr_ptr->dst_addr);


     // Validate datagrams. 
     if( ( chsum != 0 ) || ( (ip_hdr_ptr->ver_hl & 0xF0) != IP_VERSION))
     {
          return;        
     }     
       
     // Checks if datagrams are fragments. If they are, they will be droped.
     if( (ip_hdr_ptr->frag & 0xBFFF) != 0 )
     {
          return;
     }
            
     // Checks and sends datagrams further to either ICMP, TCP or UDP.
     switch(ip_hdr_ptr->prot)
     {
          // Sends datagrams to ICMP.
          case IP_PROTOCOL_ICMP:
               icmp_recv(ip_hdr_ptr, (icmp_header *) ( (uint8_t *)(ip_hdr_ptr) +   ( (ip_hdr_ptr->ver_hl & 0x0F) * 4) ) );
               break;

          // Sends datagrams to TCP.
          case IP_PROTOCOL_TCP:
               tcp_recv(ip_hdr_ptr, (tcp_header *) ((uint8_t *)(ip_hdr_ptr) + ( (ip_hdr_ptr->ver_hl & 0x0F) * 4) ) );
               break;

          // Sends datagrams to UDP.
          case IP_PROTOCOL_UDP:
               //udp_recv(ip_ptr, (udp_header *)IP_PACKET_DATA(ip_h));
               udp_recv(ip_hdr_ptr, (udp_header *) ((uint8_t *)(ip_hdr_ptr) + ( (ip_hdr_ptr->ver_hl & 0x0F) * 4) ) );
               break;  
     }
}
