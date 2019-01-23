/*   icmp.c - Internet Control Message Protocol

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/


#include "icmp.h"
#include "dhcp.h"
#include "sock.h"
#include "sys.h"



//------------------------------------------------------------------------
// icmp_send:
// This fucntions is called by icmp_recv() to send ICMP replies. After 
// all, this system does not have any terminal or such so there are no
// ping applications developed. So, this ICMP module is designed to only
// send ICMP reply to hosts and receive ICMP requests.
//------------------------------------------------------------------------
void icmp_send(const uint8_t *hw_addr, uint32_t ip_addr, uint8_t type, uint8_t code, uint16_t len)
{
     len = len + ICMP_HDR_LEN;

     // Fills the datastructure for ICMP
     icmp_hdr_tx_type = type;
     icmp_hdr_tx_code = code;
     icmp_hdr_tx_csum = 0;
  
     // Calculate checksum
     icmp_hdr_tx_csum = checksum(icmp_hdr_tx, len);
  
     // Sends ICMP messages to IP layer.
     ip_send(hw_addr, ip_addr, IP_PROTOCOL_ICMP, len);
}



//------------------------------------------------------------------------
// icmp_recv:
// This function does only one thing: sends replies on ICMP requests.
//------------------------------------------------------------------------
void icmp_recv(ip_header *ip_hdr_ptr, icmp_header *icmp_hdr_ptr)
{
     uint16_t chsum;
     
     // Checks if we really can receive anything right this minute.
     if(dhcp_check() == FALSE)
     {
          return;   
     }

     // Calculate checksum
     chsum = checksum(icmp_hdr_ptr, ( ip_hdr_ptr->len - ( ( ip_hdr_ptr->ver_hl & 0x0F) * 4 ) ) );


     // Checks if the destination for the message is local host
     if( ( chsum != 0 ) || ( ip_hdr_ptr->dst_addr != lo_ip_addr ) )
     {
          return;   
     }

     
     // Inverstigates the type field in ICMP messages
     switch(icmp_hdr_ptr->type)
     {
          // The local host received an ICMP request then it send
          // an ICMP reply
          case ICMP_TYPE_ECHO_REQUEST:
               memcpy(icmp_data_tx, (uint8_t *)icmp_hdr_ptr + ICMP_HDR_LEN, ( ip_hdr_ptr->len - ( ( ip_hdr_ptr->ver_hl & 0x0F ) * 4 ) ) - ICMP_HDR_LEN );
               icmp_send(rx_buf.src, ip_hdr_ptr->src_addr, ICMP_TYPE_ECHO_REPLY, 0, ( ip_hdr_ptr->len - ( ( ip_hdr_ptr->ver_hl & 0x0F ) * 4 ) ) - ICMP_HDR_LEN );
               break;
     }                
}
