/*   main.c - Main

     Högskolan Dalarna / Dalarna University
     Jonas Olsson
     h02jonol(at)du.se
*/

#include "timer.h"
#include "disp.h"
#include "dhcp.h"
#include "sock.h"
#include "http.h"
#include "tcp.h"
#include "sys.h"



void main(void)
{        
     uint16_t cur_sock, len;
     tcp_socket *socket;

     timer_init();
     disp_init();
     
     disp_print("wait..");

     sock_init();   
     while(!dhcp_check());
     
     disp_print("online!");
 
   

     for(cur_sock = 0; ; cur_sock)
     {    
          socket = &lo_sock[cur_sock];
          
          //if(sock_try_lock())
          //{
               sock_set_lock(TRUE);
               len = (uint16_t)(socket->rx_buf_end - socket->rx_buf_beg);
               sock_set_lock(FALSE);
          //}

          if((lo_sock[cur_sock].state == TCP_STATE_ESTABLISHED) && (len > 0))
          {
               http_request(cur_sock);
          }
          else if(lo_sock[cur_sock].state == TCP_STATE_CLOSED)
          {
               sock_listen(cur_sock, HTTP_SERVER_PORT);
          }

          if(cur_sock < SOCK_NUM)
          {
               cur_sock = cur_sock + 1;
          }
          else
          {
               cur_sock = 0;
          }
     }
      
}
