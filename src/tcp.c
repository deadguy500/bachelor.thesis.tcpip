/*   tcp.c - Transmission Control Protocol

     Högskolan Dalarna / Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/


#include "timer.h"
#include "tcp.h"
#include "sock.h"
#include "sys.h"
#include "dhcp.h"



tcp_socket lo_sock[SOCK_NUM];

// Take a look at sock module for description of these
extern bool rx_wait;       
extern bool tx_wait;
extern uint16_t sock_wait;   



//------------------------------------------------------------------------
// tcp_init:
// This function initiate TCP communication by starting up SOCK_NUM 
// different sockets to receive and transmit information by.
//------------------------------------------------------------------------
void tcp_init(void)
{
     uint16_t i;

     for(i = 0; i < SOCK_NUM; i++)
     {
          tcp_socket *sock = &lo_sock[i];
          sock->id = i;
          sock->state = TCP_STATE_CLOSED;
          sock->rx_buf_beg = 0;
          sock->rx_buf_end = 0;
     }
}



//------------------------------------------------------------------------
// tcp_set_state:
// Because TCP communication with two nodes uses a state machine, we also
// need a function which represent this state machine, and tcp_set_state()
// is this function.
//
// Note: Think like a server, be like a server, serv as a server... thats
// is the philosophy of this function.
//------------------------------------------------------------------------
void tcp_set_state(void)
{
     uint16_t i, beg_tx, end_tx, len, len_c;
     uint8_t *data;
  
     // We got SOCK_NUM socket, so therefor we
     // need to check the state for each socket.
     for(i = 0; i < SOCK_NUM; i++)
     {
          tcp_socket *sock = &lo_sock[i];
    
          switch(sock->state)
          {
               // This is the starting point of the TCP state diagram.
               case TCP_STATE_CLOSING:
                    sock->state = TCP_STATE_CLOSED;      
                    break;               
               
               // A client sent a SYN-package
               case TCP_STATE_SYN_REC:
                    if(secs >= sock->timeout)
                    {
                         if(sock->retries < 4)
                         {     
                              sock->timeout = secs + (4 << sock->retries);      
                              sock->retries = sock->retries + 1;
                              tcp_send(sock, TCP_FLAG_SYN | TCP_FLAG_ACK, 0);
                         }    
                         else
                         {
                              sock->state = TCP_STATE_CLOSED;                  
                              tcp_send(sock, TCP_FLAG_RESET | TCP_FLAG_ACK, 0);     
                         }
                    }
                    break;
                    
               // Communication between client and server is
               // established. Now may information flow free.
               case TCP_STATE_ESTABLISHED:
                    if(secs >= sock->timeout)
                    {
                         // We need some kind of check of retries of connection, else
                         // we would try to connect to a node for infinity. So, we
                         // can try to connect four times before we should figure it
                         // out that the remote node is unavailable.
                         if(sock->retries < 4)
                         {     
                              sock->timeout = secs + (4 << sock->retries);      
                              sock->retries = sock->retries + 1;                
                    
                              if(sock->tx_buf_beg != sock->tx_buf_end)
                              {
                                   beg_tx = sock->tx_buf_beg & TCP_TX_BUF_LEN;
                                   end_tx = sock->tx_buf_end & TCP_TX_BUF_LEN;  
                                   len = ETHER_FRAME_LEN_MAX - ETHER_HDR_LEN - 40;
                                   
                                   if(end_tx >= beg_tx)
                                   {
                                        len_c = end_tx - beg_tx;
                                   }
                                   else
                                   {
                                        len_c = TCP_TX_BUF_LEN - beg_tx + end_tx + 1;
                                   }
  
                                   if(len > len_c)
                                   {
                                        len = len_c;
                                   }
                                   else
                                   {
                                        len_c = len;
                                   }
                                   
                                   data = tcp_data_tx;
  
                                   while(len_c != 0)
                                   {    
                                        *((uint8_t *)data)++ = sock->tx_buf[beg_tx];    
                                        beg_tx = (beg_tx + 1) & TCP_TX_BUF_LEN;
                                        len_c = len_c - 1;
                                   }                                
                                   
                                   tcp_send(sock, TCP_FLAG_ACK | TCP_FLAG_PUSH, len);    
                              }
                         }    
                         else
                         {    
                              // If we have tried four times to connect we assume that
                              // the remote node is offline or application is unavailable, then
                              // we put us self back to the closed state.
                              sock->state = TCP_STATE_CLOSED;
                              tcp_send(sock, TCP_FLAG_RESET | TCP_FLAG_ACK, 0);                               
          
                              if(sock_wait == sock->id)  
                              {
                                   rx_wait = FALSE;
                              }
                         }
                    }        
                    break;                 
               
               // At this point in the state diagram we want to close the connection
               // with the client. Do do so we sends an datagram containing FIN-flag
               // and put us self into the cloesed state.
               case TCP_STATE_LAST_ACK:
                    if(secs >= sock->timeout)
                    {
                         if(sock->retries < 4)
                         {     
                              sock->timeout = secs + (4 << sock->retries);      
                              sock->retries = sock->retries + 1;        
    
                              tcp_send(sock, TCP_FLAG_FIN | TCP_FLAG_ACK, 0);
                         }    
                         else
                         {
                              sock->state = TCP_STATE_CLOSED;                  
                              tcp_send(sock, TCP_FLAG_RESET | TCP_FLAG_ACK, 0);                               
                         }
                    }          
                    break;
          }    
     }
}



//------------------------------------------------------------------------
// tcp_recv:
// This fuction handles incoming information.
//------------------------------------------------------------------------
void tcp_recv(ip_header *ip_hdr_ptr, tcp_header *tcp_hdr_ptr)
{  
     tcp_socket *sock;
     tcp_socket *tmp_sock;
     uint16_t sock_cnt;
     uint16_t len;
     uint16_t tmp_len;
     uint8_t flags;
     uint16_t beg_rx;
     uint16_t end_rx;
     uint8_t *data;

     // Checks if the network is configurated.
     if(dhcp_check() == FALSE)
     {
          return;   
     }

     // Calculates TCP checksum.
     tcp_checksum(tcp_hdr_ptr, ip_hdr_ptr->src_addr, ip_hdr_ptr->dst_addr, (ip_hdr_ptr->len - ((ip_hdr_ptr->ver_hl & 0x0F) * 4)));

     // Checks if the checksum is ok and if destination IP address
     // in the datagram is our node.
     if((tcp_hdr_ptr->csum != 0) || (ip_hdr_ptr->dst_addr != lo_ip_addr))
     {
          return;    
     }
    
     // Change endian (network byte order)
     tcp_hdr_ptr->src_port = memswap(tcp_hdr_ptr->src_port);
     tcp_hdr_ptr->dst_port = memswap(tcp_hdr_ptr->dst_port);
     tcp_hdr_ptr->seq = memswap32(tcp_hdr_ptr->seq);
     tcp_hdr_ptr->ack = memswap32(tcp_hdr_ptr->ack);
     tcp_hdr_ptr->win = memswap(tcp_hdr_ptr->win);
     tcp_hdr_ptr->urg_ptr = memswap(tcp_hdr_ptr->urg_ptr);


     // Determin which of our sockets we are receiving information to.
     for(sock_cnt = 0; sock_cnt < SOCK_NUM; sock_cnt++)
     {
          tmp_sock = &lo_sock[sock_cnt];
    
          if( (tmp_sock->dst_ip_addr == ip_hdr_ptr->src_addr) && 
              (tmp_sock->src_port == tcp_hdr_ptr->dst_port) && 
              (tmp_sock->dst_port == tcp_hdr_ptr->src_port))
          {
               sock = tmp_sock;
               break;
          }
          else if( (tmp_sock->state == TCP_STATE_LISTEN) && 
                   (tmp_sock->src_port == tcp_hdr_ptr->dst_port))
          {
               sock = tmp_sock;
               break;
          }
          else
          {
               sock = NULL;
          }
     }

     // And if there are no one of our sockets, we drop everything.
     if(sock == NULL)
     {
          return;
     }

     // Determin length and flags
     len = (ip_hdr_ptr->len - ((ip_hdr_ptr->ver_hl & 0x0F) * 4)) - (((tcp_hdr_ptr->hlen & 0xF0) >> 2));
     flags = tcp_hdr_ptr->flags & (TCP_FLAG_FIN | TCP_FLAG_SYN | TCP_FLAG_RESET | TCP_FLAG_ACK);
  

     // This is the other part of the state diagram, discussed in the tcp_set_state()
     // function. As before, see this system implementation as a server and nothing
     // else...
     switch(sock->state)
     {
          // Start to listen for trafic by initiate the socket.
          case TCP_STATE_LISTEN:
               if(flags == TCP_FLAG_SYN)       
               {    
                    memcpy(sock->dst_hw_addr, rx_buf.src, 6);
                    sock->dst_ip_addr = ip_hdr_ptr->src_addr;
                    sock->dst_port = tcp_hdr_ptr->src_port;
                    sock->timeout = 0;           
                    sock->retries = 0;
                    sock->state = TCP_STATE_SYN_REC;                  
                    sock->rx_buf_beg = tcp_hdr_ptr->seq + 1;  
                    sock->rx_buf_end = sock->rx_buf_beg;                 
                    sock->tx_buf_beg = ticks;  
                    sock->tx_buf_end = sock->tx_buf_beg + 1;     
               }  
               break;
        
          // At this state our system received a SYN package. This
          // is to synchronize the connection between server and client.
          // To do this we send an ACK package.
          case TCP_STATE_SYN_REC:  
               if((tcp_hdr_ptr->seq == sock->rx_buf_end) && (tcp_hdr_ptr->ack == sock->tx_buf_end) && (flags == TCP_FLAG_ACK))
               {              
                    sock->timeout = 0;        
                    sock->retries = 0;
                    sock->state = TCP_STATE_ESTABLISHED;                                      
                    sock->tx_buf_beg = tcp_hdr_ptr->ack;               
      
                    // Calculate information length and fill the buffer 
                    // with information.
                    if(len > 0)  
                    {
                         beg_rx = sock->rx_buf_beg & TCP_RX_BUF_LEN;
                         end_rx = sock->rx_buf_end & TCP_RX_BUF_LEN;  
                         
                         if(beg_rx > end_rx)
                         {
                              tmp_len = beg_rx - end_rx - 1;
                         }
                         else
                         {
                              tmp_len = TCP_RX_BUF_LEN - end_rx + beg_rx;
                         }
                         
                         if(len > tmp_len)
                         {
                              len = tmp_len;
                         }
                         else
                         {
                              tmp_len = len;
                         }
                         
                         data = ((uint8_t *)(tcp_hdr_ptr) + ((tcp_hdr_ptr->hlen & 0xF0) >> 2));
                         
                         // Fills the package with information.
                         while(tmp_len != 0)
                         {    
                              sock->rx_buf[end_rx] = *((const uint8_t *)data)++;
                              end_rx = (end_rx + 1) & TCP_RX_BUF_LEN; 
                              tmp_len = tmp_len - 1;
                         } 
  
                         sock->rx_buf_end = sock->rx_buf_end + len;      
  
                         tcp_send(sock, TCP_FLAG_ACK, 0);        
                    }
               }      
               break;
    
          // As described in the tcp_set_state(), at this state can information flow
          // throw server and client. Here we calculate incoming length of information
          // and take care of the information
          case TCP_STATE_ESTABLISHED:   
               if((tcp_hdr_ptr->seq == sock->rx_buf_end) && !(flags & TCP_FLAG_SYN))
               {                               
                    if(flags & TCP_FLAG_ACK)
                    {               
                         sock->timeout = 0;    
                         sock->retries = 0;
      
                         sock->tx_buf_beg = tcp_hdr_ptr->ack;            

                         if(sock_wait == sock->id)
                         {
                              tx_wait = FALSE;
                         }
                    }

                    if(flags & TCP_FLAG_RESET)
                    {    
                         sock->state = TCP_STATE_CLOSED;

                         if(sock_wait == sock->id)        
                         {
                              rx_wait = FALSE;
                         }
                    }
                    else if(flags & TCP_FLAG_FIN)
                    {    
                         sock->state = TCP_STATE_LAST_ACK;                   
        
                         if(sock_wait == sock->id)
                         {
                              rx_wait = FALSE;
                         }
                    }

                    // Calculate the length
                    if(len > 0)
                    {
                         beg_rx = sock->rx_buf_beg & TCP_RX_BUF_LEN;
                         end_rx = sock->rx_buf_end & TCP_RX_BUF_LEN;  
                         
                         if(beg_rx > end_rx)
                         {
                              tmp_len = beg_rx - end_rx - 1;
                         }
                         else
                         {
                              tmp_len = TCP_RX_BUF_LEN - end_rx + beg_rx;
                         }
                         
                         if(len > tmp_len)
                         {
                              len = tmp_len;
                         }
                         else
                         {
                              tmp_len = len;
                         }
                         
                         data = ((uint8_t *)(tcp_hdr_ptr) + ((tcp_hdr_ptr->hlen & 0xF0) >> 2));
                         
                         // Fetches the information.
                         while(tmp_len != 0)
                         {    
                              sock->rx_buf[end_rx] = *((const uint8_t *)data)++;
                              end_rx = (end_rx + 1) & TCP_RX_BUF_LEN; 
                              tmp_len = tmp_len - 1;
                         } 
  
                         sock->rx_buf_end = sock->rx_buf_end + len;      
                          
                         // Sends an acknowledgement package so the remote node knows that
                         // we got the information.
                         tcp_send(sock, TCP_FLAG_ACK, 0);
        
                         if(sock_wait == sock->id)
                         {
                              rx_wait = FALSE;
                         }
                    }
               }    
               break;
    
          // Received a FIN package, this means that quite soon this connection
          // will be terminated. By synchronize the termination we send a ACK package.
          case TCP_STATE_FIN_WAIT:  
               if((tcp_hdr_ptr->seq == sock->rx_buf_end) && (tcp_hdr_ptr->ack == sock->tx_buf_end + 1) && (flags & TCP_FLAG_FIN))
               {        
                    sock->state = TCP_STATE_CLOSING;        
                    tcp_send(sock, TCP_FLAG_ACK, 0);                 
               }
               break;
    
          // Received the last ACK package, time to close connection.
          case TCP_STATE_LAST_ACK:  
               if((tcp_hdr_ptr->seq == sock->rx_buf_end + 1) && (tcp_hdr_ptr->ack == sock->tx_buf_end + 1) && (flags == TCP_FLAG_ACK))
               {
                    sock->state = TCP_STATE_CLOSED;            
               }
               break;          
     }     
}



//------------------------------------------------------------------------
// tcp_send:
// This is similar to every send function in this system; it fills the
// data structure with information
//------------------------------------------------------------------------
void tcp_send(tcp_socket *sock, uint8_t flags, uint16_t len)
{
     len = len + TCP_HDR_LEN;   

     // Fill the datastructure with information
     tcp_hdr_tx_src_port = sock->src_port;
     tcp_hdr_tx_dst_port = sock->dst_port;
     tcp_hdr_tx_seq = sock->tx_buf_beg;
     tcp_hdr_tx_ack = sock->rx_buf_end + (sock->state & TCP_STATE_FLAG_RX_FIN ? 1 : 0);
     tcp_hdr_tx_hlen = TCP_HDR_LEN << 2;
     tcp_hdr_tx_flags = flags;
     tcp_hdr_tx_win = TCP_RX_BUF_LEN - (uint16_t)(sock->rx_buf_end - sock->rx_buf_beg);
     tcp_hdr_tx_csum = 0;
     tcp_hdr_tx_urg_ptr = 0;    
  
     // Change endian
     tcp_hdr_tx_src_port = memswap(tcp_hdr_tx_src_port);
     tcp_hdr_tx_dst_port = memswap(tcp_hdr_tx_dst_port);
     tcp_hdr_tx_seq = memswap32(tcp_hdr_tx_seq);
     tcp_hdr_tx_ack = memswap32(tcp_hdr_tx_ack);
     tcp_hdr_tx_win = memswap(tcp_hdr_tx_win);
     tcp_hdr_tx_urg_ptr = memswap(tcp_hdr_tx_urg_ptr);
  
     // Calculate TCP checksum
     tcp_checksum(tcp_hdr_tx, lo_ip_addr, sock->dst_ip_addr, len);
    
     // Send it down to the IP layer.
     ip_send(sock->dst_hw_addr, sock->dst_ip_addr, IP_PROTOCOL_TCP, len);
}



//------------------------------------------------------------------------
// tcp_send:
// This function works exactly the same as checksum calculation for
// UDP.
//------------------------------------------------------------------------
void tcp_checksum(tcp_header *tcp_hdr, uint32_t src, uint32_t dest, uint16_t len)
{
     uint32_t sum = 0;
     
     // Change byte order
     src = memswap32(src);
     dest = memswap32(dest);
  
     // Summarize the information
     sum = memswap(len);
     sum = sum + memswap((uint16_t)IP_PROTOCOL_TCP); 
     sum = sum + ((uint16_t *)&src)[0];
     sum = sum + ((uint16_t *)&src)[1];
     sum = sum + ((uint16_t *)&dest)[0];
     sum = sum + ((uint16_t *)&dest)[1];
     sum = sum + ~checksum(tcp_hdr, len);

     while(sum >> 16)
     {
          sum = (sum & 0xffff) + (sum >> 16);
     }

     tcp_hdr->csum = ~(uint16_t)sum; 
}
