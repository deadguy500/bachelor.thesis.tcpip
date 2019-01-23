/*   sock.c - Sockets

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se

     Note: I attempted to develop this module like a ordinary socket-implementations,
           such as its just a integer to refer to when to communicate with other
           nodes on a network. But this did not go so well, so this is the result; a 
           part of a real socket implementation.
*/

#include "ether.h"
#include "tcp.h"
#include "sock.h"
#include "sys.h"
#include "dhcp.h"



uint8_t  bc_hw_addr[6]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint32_t bc_ip_addr     = 0xFFFFFFFF;
uint8_t  lo_hw_addr[6]  = {0x00, 0x90, 0x27, 0x93, 0xEB, 0x65}; 
uint32_t lo_ip_addr;
uint8_t gw_hw_addr[6];
uint32_t gw_ip_addr;
uint32_t sn_mask;
uint8_t dhcp_state;



// This is like a semaphore so no other than the executing fuction
// override important information.
bool sock_lock = TRUE;

// This is to check if the gateway parameters is set for the system.
bool gw_set;

// These tow can ses as condition variables for check if there are
// any interruptions in transmitting and recieving.
bool rx_wait;
bool tx_wait;
uint16_t sock_wait;



//------------------------------------------------------------------------
// sock_init:
// This function initiate the whole TCP/IP stack by starting ethernet
// and TCP modules.
//------------------------------------------------------------------------
void sock_init(void)
{   
     sock_lock = TRUE;
  
     dhcp_state = DHCP_STATE_INIT;                    
  
     gw_set = FALSE;
     rx_wait = FALSE;
     tx_wait = FALSE;       
     sock_wait = 1;
 
     ether_init();
     tcp_init();     
  
     sock_lock = FALSE;  
}



//------------------------------------------------------------------------
// sock_try_lock:
// This function checks if the semaphore is taken or not. Returns TRUE
// if its free, else FALSE.
//------------------------------------------------------------------------
bool sock_try_lock(void)
{    
     return !sock_lock;
}


//------------------------------------------------------------------------
// sock_set_lock:
// This function sets the socket lock.
//------------------------------------------------------------------------
//void sock_set_lock(uint16_t lock, bool lock_state)
void sock_set_lock(bool lock_state)
{    
     sock_lock = lock_state;
}


//------------------------------------------------------------------------
// sock_set_gw:
// This fuction declare that the gateway parameters are available
//------------------------------------------------------------------------
void sock_set_gw(bool gw_state)
{
     //gw_set = TRUE;
     gw_set = gw_state;
}



//------------------------------------------------------------------------
// sock_check_gw:
// This function checks if the gateway parameters are available
//------------------------------------------------------------------------
bool sock_check_gw(void)
{
     return gw_set;
}



//------------------------------------------------------------------------
// sock_listen:
// This function is called to see if there are any new connections 
// how wants to get information from us.
//
// With other words, when we call this function its start to listen if
// there are any incoming information sent to the socket.
//------------------------------------------------------------------------
void sock_listen(uint16_t cur_sock, uint16_t port)
{    
     tcp_socket *socket;
     
     socket = &lo_sock[cur_sock];
     sock_lock = TRUE;
     
     if((socket->state != TCP_STATE_CLOSED) && (socket->state != TCP_STATE_LISTEN))
     {    
          socket->state = TCP_STATE_CLOSED;          
          tcp_send(socket, TCP_FLAG_RESET | TCP_FLAG_ACK, 0); 
     }
    
     socket->state = TCP_STATE_LISTEN;  
     socket->src_port = port;
     
     sock_lock = FALSE;
}



//------------------------------------------------------------------------
// sock_read:
// Reads information from the TCP socket
//------------------------------------------------------------------------
uint16_t sock_read(uint16_t cur_sock, void *data, uint16_t len)
{
     uint16_t beg_rx, end_rx, tmp_len;
     tcp_socket *socket;
          
     socket = &lo_sock[cur_sock];
     sock_lock = TRUE;
  
      
     // Start to read the recieve buffer...
     if(socket->state & TCP_STATE_FLAG_RX_BUF)
     {
          // ...and we can only do so when we are in the
          // established state..
          while(socket->state == TCP_STATE_ESTABLISHED)
          {
               if(socket->rx_buf_beg != socket->rx_buf_end)
               {
                    break;
               }
    
               rx_wait = TRUE;
               sock_wait = cur_sock; 
               sock_lock = FALSE;

               while(rx_wait);

               sock_lock = TRUE;  
          }

          // Calculates the length of the recieving information
          beg_rx = socket->rx_buf_beg & TCP_RX_BUF_LEN;
          end_rx = socket->rx_buf_end & TCP_RX_BUF_LEN;  

          if(end_rx >= beg_rx)
          {
               tmp_len = end_rx - beg_rx;
          }
          else
          {
               tmp_len = TCP_RX_BUF_LEN - beg_rx + end_rx + 1;
          }
  
          if(len > tmp_len)
          {
               len = tmp_len;
          }
          else
          {
               tmp_len = len;
          }
  
          // Fetchs the information
          while(tmp_len != 0)
          {    
               *((uint8_t *)data)++ = socket->rx_buf[beg_rx];
               beg_rx = (beg_rx + 1) & TCP_RX_BUF_LEN;
               tmp_len = tmp_len - 1;
          } 
  
          socket->rx_buf_beg = socket->rx_buf_beg + len;          
     }
     else
     {
          len = 0;
     }

     sock_lock = FALSE;

     return len;
}



//------------------------------------------------------------------------
// sock_write:
// Write information to the TCP socket
//------------------------------------------------------------------------
void sock_write(uint16_t cur_sock, const void *data, uint16_t len)
{
     uint16_t beg_tx, end_tx, tmp_len;
     tcp_socket *socket; 
     
     socket = &lo_sock[cur_sock];
     sock_lock = TRUE;    
    
     // We need to be in the established state to write any information
     // to a socket.
     while(socket->state == TCP_STATE_ESTABLISHED)
     {    
          // Calculate the length of the output buffer
          beg_tx = socket->tx_buf_beg & TCP_TX_BUF_LEN;
          end_tx = socket->tx_buf_end & TCP_TX_BUF_LEN;

          if(beg_tx > end_tx)
          {
               tmp_len = beg_tx - end_tx - 1;
          }
          else
          {
               tmp_len = TCP_TX_BUF_LEN - end_tx + beg_tx;
          }
          
          if(len > tmp_len)
          {
               len = tmp_len;
          }
          else
          {
               tmp_len = len;
          }
  
          // Write all the data to the socket for futher transportation.
          while(tmp_len != 0)
          {    
               socket->tx_buf[end_tx] = *((const uint8_t *)data)++;
               end_tx = (end_tx + 1) & TCP_TX_BUF_LEN; 
               tmp_len = tmp_len - 1;
          }     
  
          if(len > 0)
          {
               socket->timeout = 0;         
               socket->retries = 0;    
               socket->tx_buf_end = socket->tx_buf_end + len;
          }
          
          tmp_len = len;
        
          (const uint8_t *)data += tmp_len;

          // Checks if there are more information to transmit.
          len = len - tmp_len;      
        
          if(len == 0)
          {
               break;      
          }

          tx_wait = TRUE;
          sock_wait = cur_sock;   
        
          sock_lock = FALSE;

          while(tx_wait);

          sock_lock = TRUE;                 
     }
  
     sock_lock = FALSE;
}                               

 
