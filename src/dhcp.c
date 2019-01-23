/*   dhcp.c - Dynamic Host Configure Protocol

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/



#include "timer.h"
#include "arp.h"
#include "dhcp.h"
#include "sock.h"
#include "sys.h"





static uint32_t xid;          // Transaction ID
static uint32_t req_ip;       // Requested IP
static uint32_t server_ident; // Server identification
static uint8_t retries;       // Nums of retries
static uint32_t timeout;      // Timout value


  


//------------------------------------------------------------------------
// dhcp_state_set:
// DHCP uses a state diagram to describe the connection between the 
// client and server. This function keep track of which state in the
// communication we are in.
//------------------------------------------------------------------------
void dhcp_set_state(void)
{
     switch(dhcp_state)  
     {
          // This is the initial state of the communication. Here sets
          // the information, which the system later will ask the dhcp
          // server, to null. Such information is IP address, gateway
          // IP address, subnet mask...
          case DHCP_STATE_INIT:
               lo_ip_addr = 0;
               gw_ip_addr = 0;
               memclr(gw_hw_addr, 6);
               sn_mask = 0;
               dhcp_state = DHCP_STATE_SELECTING;         
               xid = ~ticks;                 
               timeout = 0;
               retries = 0;        
    
          // After initiaton is done the system need to send a
          // message called DHCPDISCOVER to see if there are any
          // DHCP-servers out there.
          case DHCP_STATE_SELECTING:
               if(secs >= timeout)
               {
                    if(retries < 4)
                    {    
                         timeout = secs + (4 << retries);
                         retries += 1;      
                         dhcp_send(DHCPDISCOVER);  
                    }
                    else
                    {
                         dhcp_state = DHCP_STATE_INIT;    
                    }
               }
               break; 
  
          // After a while, if there are any DHCP server on the network,
          // we recieve information atleast from one DHCP-server. If there
          // are more than one DHCP server on the network must we chose one.
          // The server answers with a DHCPPACK containing configuration 
          // information for the system.
          case DHCP_STATE_REQUESTING:
               if(secs >= timeout)
               {
                    if(retries < 4)
                    {        
                         timeout = secs + (4 << retries);
                         retries += 1;
                         dhcp_send(DHCPREQUEST);
                    }
                    else
                    {
                         dhcp_state = DHCP_STATE_INIT;
                    }
               }
               break;           
  
          // At last, this is the last state in the DHCP diagram. From
          // here we can start send and receive information from other
          // nodes on the network and Internet.
          case DHCP_STATE_BOUND:
               if(!sock_check_gw() && (secs >= timeout))
               {    
                    timeout = secs + (4 << 0);
                    arp_send(bc_hw_addr, gw_ip_addr, ARP_OP_REQUEST);
               }
               break;
     }        
}



//------------------------------------------------------------------------
// dhcp_recv:
// This function describes how the DHCP client receives information from
// a DHCP server.
//------------------------------------------------------------------------
void dhcp_recv(udp_header *udp_hdr_ptr, dhcp_packet *dhcp_pkt_ptr)
{
     uint8_t *opt_ptr;
     uint8_t *opt_end;
     uint32_t opt_sn_mask = 0;
     uint32_t opt_gw = 0;
     uint8_t opt_msg_type = 0;
     uint32_t opt_server_ident = 0;

     opt_ptr = dhcp_pkt_ptr->options;
     opt_end = (uint8_t *)dhcp_pkt_ptr + (udp_hdr_ptr->len - UDP_HDR_LEN);  

     // Change endian..
     dhcp_pkt_ptr->xid = memswap32(dhcp_pkt_ptr->xid);
     dhcp_pkt_ptr->secs = memswap(dhcp_pkt_ptr->secs);
     dhcp_pkt_ptr->flags = memswap(dhcp_pkt_ptr->flags);
     dhcp_pkt_ptr->ciaddr = memswap32(dhcp_pkt_ptr->ciaddr);
     dhcp_pkt_ptr->yiaddr = memswap32(dhcp_pkt_ptr->yiaddr);
     dhcp_pkt_ptr->siaddr = memswap32(dhcp_pkt_ptr->siaddr);
     dhcp_pkt_ptr->giaddr = memswap32(dhcp_pkt_ptr->giaddr);

     // Validate if the package is correct.  
     if( (dhcp_pkt_ptr->op != DHCP_OP_BOOT_REPLY) || 
         (dhcp_pkt_ptr->htype != DHCP_TYPE_ETHER) || 
         (dhcp_pkt_ptr->hlen != 6) ||
         (dhcp_pkt_ptr->xid != xid) ||
         (opt_ptr[0] != DHCP_OPT_MAGIC_COOKIE_0) ||
         (opt_ptr[1] != DHCP_OPT_MAGIC_COOKIE_1) ||
         (opt_ptr[2] != DHCP_OPT_MAGIC_COOKIE_2) ||
         (opt_ptr[3] != DHCP_OPT_MAGIC_COOKIE_3))
     {
          return;        
     }
              
     opt_ptr = opt_ptr + 4;
  

     // Here we checks what information the option field contains.
     while(opt_ptr < opt_end)
     {
          switch(*opt_ptr++)
          {
               // Ignore all pad information.
               case DHCP_OPT_PAD:
                    ++opt_ptr;
                    continue;
      
               case DHCP_OPT_END:
                    opt_end = ++opt_ptr;
                    continue;                  
    
               // Save the gateway IP address
               case DHCP_OPT_GW:
                    ((uint8_t *)&opt_gw)[3] = opt_ptr[1];
                    ((uint8_t *)&opt_gw)[2] = opt_ptr[2];
                    ((uint8_t *)&opt_gw)[1] = opt_ptr[3];
                    ((uint8_t *)&opt_gw)[0] = opt_ptr[4];
                    break;
      
               // Save the subnet mask
               case DHCP_OPT_SN_MASK:
                    ((uint8_t *)&opt_sn_mask)[3] = opt_ptr[1];
                    ((uint8_t *)&opt_sn_mask)[2] = opt_ptr[2];
                    ((uint8_t *)&opt_sn_mask)[1] = opt_ptr[3];
                    ((uint8_t *)&opt_sn_mask)[0] = opt_ptr[4];    
                    break;      
      
               // Save the message type
               case DHCP_OPT_MSG_TYPE:
                    opt_msg_type = opt_ptr[1];
                    break;
               
               // Save the server ident
               case DHCP_OPT_SERVER_IDENT:
                    ((uint8_t *)&opt_server_ident)[3] = opt_ptr[1];
                    ((uint8_t *)&opt_server_ident)[2] = opt_ptr[2];
                    ((uint8_t *)&opt_server_ident)[1] = opt_ptr[3];
                    ((uint8_t *)&opt_server_ident)[0] = opt_ptr[4];      
                    break;            
          }
    
          opt_ptr = opt_ptr + *opt_ptr + 1;
     }
    
     
     // Determin what kind of message the package is.
     switch(opt_msg_type)     
     {    
          // The DHCPOFFER package containing configuration information
          // for the system.
          case DHCPOFFER:                           
               if(dhcp_state == DHCP_STATE_SELECTING)
               {      
                    dhcp_state = DHCP_STATE_REQUESTING;                  
                    timeout = 0;
                    retries = 0;               
                    req_ip = dhcp_pkt_ptr->yiaddr;
                    server_ident = opt_server_ident;                                                      
               }
               break;  

          // This is an acknowledge message from the server to our
          // system that tells us that the server agreed to let us
          // use the configuration parameters.
          case DHCPACK:    
               if(dhcp_state == DHCP_STATE_REQUESTING)
               {
                    if(server_ident != opt_server_ident)
                    {
                         return;
                    }
      
                    dhcp_state = DHCP_STATE_BOUND;
                    lo_ip_addr = dhcp_pkt_ptr->yiaddr;
                    gw_ip_addr = opt_gw;
                    sn_mask = opt_sn_mask;
                    timeout = 0;
                    sock_set_gw(FALSE);     
                    
                    arp_send(bc_hw_addr, bc_ip_addr, ARP_OP_REPLY);                    
               }
               break;

          // If the server do not agree to let us use the configuration
          // information it will send us this message. 
          // NACK = not acknowledged!
          case DHCPNACK:  
               if((dhcp_state == DHCP_STATE_REQUESTING) || (dhcp_state == DHCP_STATE_BOUND))
               {
                    if(server_ident != opt_server_ident)
                    {
                         return;
                    }
            
                    dhcp_state = DHCP_STATE_INIT;                  
               }
               break;      
     }        
}



//------------------------------------------------------------------------
// dhcp_send:
// To send a request or discovery message we need this function.
// To see the difference with other protocols we do not need any kind of
// checksum to send DHCP messages because DHCP works with UDP, which do
// not use any reliability or validation in terms as TCP does.
//------------------------------------------------------------------------
void dhcp_send(uint8_t type)
{
     uint8_t *opt_ptr = dhcp_tx_pkt_options;

     // Fill the datastructure with information
     dhcp_tx_pkt_op = DHCP_OP_BOOT_REQUEST;
     dhcp_tx_pkt_htype = DHCP_TYPE_ETHER;
     dhcp_tx_pkt_hlen = 6;
     dhcp_tx_pkt_hops = 0;
     dhcp_tx_pkt_xid = xid;
     dhcp_tx_pkt_secs = 0;
     dhcp_tx_pkt_flags = 0;
     dhcp_tx_pkt_ciaddr = 0;
     dhcp_tx_pkt_yiaddr = 0;
     dhcp_tx_pkt_siaddr = 0;
     dhcp_tx_pkt_giaddr = 0;
     memcpy(dhcp_tx_pkt_chaddr, lo_hw_addr, 6);  
     memclr(dhcp_tx_pkt_chaddr + 6, 10);    
     memclr(dhcp_tx_pkt_sname,64);
     memclr(dhcp_tx_pkt_file, 128);  

     // Change endian..
     dhcp_tx_pkt_xid = memswap32(dhcp_tx_pkt_xid);
     dhcp_tx_pkt_secs = memswap(dhcp_tx_pkt_secs);
     dhcp_tx_pkt_flags = memswap(dhcp_tx_pkt_flags);
     dhcp_tx_pkt_ciaddr = memswap32(dhcp_tx_pkt_ciaddr);
     dhcp_tx_pkt_yiaddr = memswap32(dhcp_tx_pkt_yiaddr);
     dhcp_tx_pkt_siaddr = memswap32(dhcp_tx_pkt_siaddr);
     dhcp_tx_pkt_giaddr = memswap32(dhcp_tx_pkt_giaddr);
      
     // Fills some of the option fields with so called Magic cookies.
     *opt_ptr++ = DHCP_OPT_MAGIC_COOKIE_0;
     *opt_ptr++ = DHCP_OPT_MAGIC_COOKIE_1;
     *opt_ptr++ = DHCP_OPT_MAGIC_COOKIE_2;
     *opt_ptr++ = DHCP_OPT_MAGIC_COOKIE_3;
     *opt_ptr++ = DHCP_OPT_MSG_TYPE;
     *opt_ptr++ = 1;
     *opt_ptr++ = type;

     // There are two kinds of messages we can send: Discover and request.
     // And here we looks at 'type' and see what kind we are going to send.
     switch(type)
     {
          case DHCPDISCOVER:
               *opt_ptr++ = DHCP_OPT_PARAM_REQUEST;
               *opt_ptr++ = 5;
               *opt_ptr++ = DHCP_OPT_SN_MASK;
               *opt_ptr++ = DHCP_OPT_GW;
               *opt_ptr++ = DHCP_OPT_LEASE_TIME;
               *opt_ptr++ = DHCP_OPT_T1_VALUE;
               *opt_ptr++ = DHCP_OPT_T2_VALUE;    
               break;

          case DHCPREQUEST:
               *opt_ptr++ = DHCP_OPT_PARAM_REQUEST;
               *opt_ptr++ = 5;                     
               *opt_ptr++ = DHCP_OPT_SN_MASK;  
               *opt_ptr++ = DHCP_OPT_GW;       
               *opt_ptr++ = DHCP_OPT_LEASE_TIME;   
               *opt_ptr++ = DHCP_OPT_T1_VALUE;     
               *opt_ptr++ = DHCP_OPT_T2_VALUE;     
               *opt_ptr++ = DHCP_OPT_REQ_IP;
               *opt_ptr++ = 4;
               *opt_ptr++ = ((uint8_t *)&req_ip)[3];
               *opt_ptr++ = ((uint8_t *)&req_ip)[2];
               *opt_ptr++ = ((uint8_t *)&req_ip)[1];
               *opt_ptr++ = ((uint8_t *)&req_ip)[0]; 
               *opt_ptr++ = DHCP_OPT_SERVER_IDENT;
               *opt_ptr++ = 4;
               *opt_ptr++ = ((uint8_t *)&server_ident)[3];
               *opt_ptr++ = ((uint8_t *)&server_ident)[2];
               *opt_ptr++ = ((uint8_t *)&server_ident)[1];
               *opt_ptr++ = ((uint8_t *)&server_ident)[0];    
               break;
     }

  
     *opt_ptr++ = DHCP_OPT_END;
     
     udp_send(bc_hw_addr, bc_ip_addr, UDP_PORT_DHCP_CLIENT, UDP_PORT_DHCP_SERVER, opt_ptr - (uint8_t *)dhcp_tx_pkt);  
}
  


//------------------------------------------------------------------------
// dhcp_check:
// Other modules, such as ARP, need to know if it can send and recieve
// information. To figure this out it calls this function.
//------------------------------------------------------------------------
bool dhcp_check(void)
{
     if(dhcp_state != DHCP_STATE_BOUND)
     {
          return FALSE;
     }
     else
     {
          return TRUE;
     }
}



