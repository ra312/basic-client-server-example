/**************************************************************************/
/* Generic client example is used with connection-oriented server designs */
/**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#define SERVER_PORT  12345
class client
{
private:
   /* data */
public:
   int    len, rc;
   int    sockfd;
   char   send_buf[80];
   char   recv_buf[80];
   struct sockaddr_in6   addr;
   int PORT;
   client(int);
   ~client();
};

client::client(int port)
{
   PORT = 0;
}

client::~client()
{
}

int main (int argc, char *argv[])
{
   int    len, rc;
   int    sockfd;
   char   send_buf[80];
   char   recv_buf[80];
   struct sockaddr_in6   addr;

   /*************************************************/
   /* Create an AF_INET6 stream socket              */
   /*************************************************/
   sockfd = socket(AF_INET6, SOCK_STREAM, 0);
   if (sockfd < 0)
   {
      perror("socket");
      exit(-1);
   }

   /*************************************************/
   /* Initialize the socket address structure       */
   /*************************************************/
   memset(&addr, 0, sizeof(addr));
   addr.sin6_family      = AF_INET6;
   memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
   addr.sin6_port        = htons(SERVER_PORT);

   /*************************************************/
   /* Connect to the server                         */
   /*************************************************/
   rc = connect(sockfd,
                (struct sockaddr *)&addr,
                sizeof(struct sockaddr_in6));
   if (rc < 0)
   {
      perror("connect");
      close(sockfd);
      exit(-1);
   }
   printf("Connect completed.\n");
   while (1)
   {
   /*************************************************/
   /* Enter data buffer that is to be sent          */
   /*************************************************/
   memset(send_buf, '\0', sizeof(char)*80);
   for (int k=0; k<80; k++)
   {
      send_buf[k]='\0';
   }
   printf("Enter message to be sent:\n");
   gets(send_buf);

   /*************************************************/
   /* Send data buffer to the worker job            */
   /*************************************************/
   len = send(sockfd, send_buf, strlen(send_buf) + 1, 0);
   if (len != strlen(send_buf) + 1)
   {
      perror("send");
      
      
   }
   
   printf("%d bytes sent\n", len);
      if (len==2)
      { 
         if (send_buf[0]=='%')
         {
            break;
            close(sockfd);
            exit(-1);
         }
      }
   /*************************************************/
   /* Receive data buffer from the worker job       */
   /*************************************************/
   len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
   if (len != strlen(send_buf) + 1)
   {
      perror("recv");
      // close(sockfd);
      // exit(-1);
   }
   printf("%d bytes received\n", len);
   }
   /*************************************************/
   /* Close down the socket                         */
   /*************************************************/
   close(sockfd);
   return 0;
}