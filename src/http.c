/*   http.c - Webserver

     Dalarna University
     Jonas Olsson
     h02jonol(at)du.se

     Note: Is this the worlds smallest webserver? To get in the
           depth of a webserver I recomend you to take a look
           at:

           http://www.paulgriffiths.net/program/c/webserv.html
*/



#include <io196_kb.h>
#include "timer.h"
#include "sock.h"
#include "http.h"
#include "disp.h"
#include "sys.h"



static uint8_t web_doc[] = "<HTML><HEAD></HEAD><BODY><CENTER><H3>HELLO WORLD!</H1><BR>I´m a cute little 80C196KB microcontroller!<BR><BR></CENTER></BODY></HTML>";
static uint8_t web_hdr[] = "HTTP/1.1 200 OK\r\n Server: du.se/1.0\r\n Content-type: text\html\r\n\r\n";



//------------------------------------------------------------------------
// http_request:
// The webserver just got only one function, namely this and its 
// purpose is just to do one thing, replies on http-requests.
//------------------------------------------------------------------------
void http_request(uint16_t cur_sock)
{
     uint8_t method[9], req_uri[33];

     uint16_t i = 0; 
     uint16_t chr[4] = {0, 0, 0, 0};
     uint16_t len;

     // First we need to figure what kind of method is requested.
     while( (chr[0] = http_read_char(cur_sock)) != ' ') 
     {
          if(chr[0] == EOF)
          {
               return;
          }
  
          if(i < 8)
          {
               method[i++] = chr[0];
          }
     }

     method[i] = 0;      
     i = 0;

     // Then we need to fetch the "webpage", in this case its just
     // the root-directory.
     while((chr[0] = http_read_char(cur_sock)) != ' ') 
     {
          if(chr[0] == EOF)
          {
               return;
          }
  
          if(i < 32)
          {
               req_uri[i++] = chr[0];
          }
     }  

     req_uri[i] = 0;
          
     // By definition all webrequests and replies need to have an extra
     // empty line at the bottom of the message. Take a look at 'web_doc'
     // variable, you will see it is a extra '\n' at the end.
     // If there are no extra empty line the message will be ignored.
     while(( (chr[0] = http_read_char(cur_sock)) != '\n') || (chr[1] != '\r') || (chr[2] != '\n') || (chr[3] != '\r'))
     {              
          if(chr[0] == EOF)
          {
               return;        
          }

          chr[3] = chr[2];
          chr[2] = chr[1];   
          chr[1] = chr[0];          
     }
                 
     // Send the header and webpage to the webbrowser.
     if(strcmp("GET", method))
     {
          if(strcmp("/", req_uri))
          {          
               len = strlen(web_hdr);
               sock_write(cur_sock, web_hdr, len);

               len = strlen(web_doc);
               sock_write(cur_sock, web_doc, len);
          }
     }  
}



//------------------------------------------------------------------------
// http_read_char:
// This function is a help function to http_request to fetch just one
// character at the time.
//------------------------------------------------------------------------
uint16_t http_read_char(uint16_t cur_sock)
{
     uint8_t chr;

     if(sock_read(cur_sock, &chr, 1) > 0)
     {
          return chr;
     }
     else
     {
          return EOF;
     }
}
