/*   ether.c - Ethernet

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/



#include "cs8900a.h"
#include "cirrius.h"
#include "ether.h"
#include "sock.h"
#include "arp.h"
#include "sys.h"
#include "ip.h"



ether_frame tx_buf;
ether_frame rx_buf;



//------------------------------------------------------------------------
// ether_init:
// This initiate the ethernet controller so information can be send
// and recieve throw it.
//------------------------------------------------------------------------
void ether_init(void)
{    
     // uint8_t mac_adr;

     // First it reset it self so it starts at ground zero and
     // wait until its ready.
     PP_PTR_PORT = PP_SELF_CTL;
     PP_DATA_PORT0 = POWER_ON_RESET;  

     PP_PTR_PORT = PP_SELF_ST;

     while(!(PP_DATA_PORT0 & INIT_DONE));
  
     // Load the hardware address. The lines below are old stuff.
     write_packetpage(PP_IA1, (const uint16_t *)lo_hw_addr, 3);
     // mac_adr = (lo_hw_addr[0]) + (lo_hw_addr[1] << 8);
     // write_packetpage(PP_IA1, mac_adr);
     // mac_adr = (lo_hw_addr[2]) + (lo_hw_addr[3] << 8);
     // write_packetpage(PP_IA1 + 2, mac_adr);
     // mac_adr = (lo_hw_addr[4]) + (lo_hw_addr[5] << 8);
     // write_packetpage(PP_IA1 + 4, mac_adr);

     // Enables that it can start to send and recieve information
     PP_PTR_PORT = PP_LINE_CTL;
     PP_DATA_PORT0 = SERIAL_RX_ON | SERIAL_TX_ON;  

     PP_PTR_PORT = PP_RX_CTL;
     PP_DATA_PORT0 = RX_OK_ACCEPT | RX_IA_ACCEPT | RX_BROADCAST_ACCEPT;

     PP_PTR_PORT = PP_RX_CFG;
     PP_DATA_PORT0 = RX_OK_ENBL;  
}



//------------------------------------------------------------------------
// ether_poll:
// Because the software is designed for a small embedded system, we
// do not have any luxuries such as threads. So what we need to do is
// to poll the controller to see if there are any new information. And
// that is just what this funktion does.
//------------------------------------------------------------------------
void ether_poll(void)
{
     uint16_t event;
     uint16_t len;

     while(TRUE)
     {
          // First we checks that that the ISQ contains any interrupt...
          event = ISQ_PORT;

          if(event  == 0)
          {
               return;
          }
   
          // ...and if it does we looks at it and determen if there are any
          // new event or not.
          if( ( (event & ISQ_EVENT_MASK) != ISQ_RECEIVER_EVENT) || !(event & AUI_ONLY))
          {
               continue;
          }
          else
          {
               break;
          }
                    
          // Read the length of the incoming information...
          PP_PTR_PORT = PP_RX_LENGTH;
          len = PP_DATA_PORT0;
  
          // ...and the whole information package.
          read_packetpage(PP_RX_FRAME, (uint16_t *)&rx_buf, (len + 1) / 2);
  
          // Determin what kind of information it is, either IP or ARP.
          rx_buf.type = memswap(rx_buf.type);
          
          switch(rx_buf.type)
          {
               case ETHER_TYPE_IP:            
                    ip_recv((ip_header *)rx_buf.data);
                    break;
    
               case ETHER_TYPE_ARP:        
                    arp_recv((arp_packet *)rx_buf.data);
                    break;      
          }
     }    
}



//------------------------------------------------------------------------
// ether_send:
// It is a good idea to also send information, and this is just what this
// function does.
//------------------------------------------------------------------------
void ether_send(const uint8_t *hw_addr, uint16_t type, uint16_t len)
{
     uint16_t stat;

     len = len + ETHER_HDR_LEN;

     // Checks if the lenght of the information package is ok.
     if(len > ETHER_FRAME_LEN_MAX)
     {
          return;        
     }

     // Copy destination and our local hardware address to the data frame.
     memcpy(tx_buf.dst, hw_addr, 6);       
     memcpy(tx_buf.src, lo_hw_addr, 6);

     // Change endian.
     tx_buf.type = memswap(type); 
   
     // Tells CS8900A that its time to send some information.
     TX_CMD_PORT = TX_START_ALL_BYTES;
     TX_LENGTH_PORT = len;

     while(TRUE)
     {    
          // ..but first we need to see if we really can do that.
          PP_PTR_PORT = PP_BUS_ST;
          stat = PP_DATA_PORT0;
  
          if(stat & TX_BID_ERROR)
          {
               return;
          }
      
          if(stat & READY_FOR_TX_NOW)
          {
               break;
          }
     }

     // At last we write information to the transmitt buffer at the CS8900A so
     // it can send it away.
     write_packetpage(PP_TX_FRAME, (const uint16_t *)&tx_buf, (len + 1) / 2);  
}
