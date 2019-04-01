
#include <stdio.h>
#include <ostream>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#define TRUE             1
#define FALSE            0
class server_class
{
   public:
   /* data */
   std::string time; //server time
   std::string return_current_time_and_date();
   
   int    PORT;
   int    i, len, rc, on = 1;
   int    listen_sd, max_sd, new_sd;
   int    desc_ready, end_server = FALSE;
   int    close_conn;
   char   buffer[80];
   struct sockaddr_in6 addr;
   struct timeval      timeout;
   //set of file descriptors/network sockets
   fd_set              master_set, working_set;
   server_class(int);
   int create_socket();
   int set_socket_non_blocking();
   int bind_socket();
   int listen_back_log();
   int init_fd_sets();
   int set_timeout(int );
   int get_ready_incoming();
   int clean_connection(int);
   int receive_message();
   ~server_class();
};

server_class::server_class(int port)
{
   PORT = port;
   on = 1;
   end_server = FALSE;
   time = return_current_time_and_date();
}
std::string server_class::return_current_time_and_date()
{
   auto now = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);
   std::stringstream ss;
   ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
   return ss.str();
   
}
int server_class::create_socket()
{
   /*************************************************************/
   /* Create an AF_INET6 stream socket to receive incoming      */
   /* connections on                                            */
   /*************************************************************/
   listen_sd = socket(AF_INET6, SOCK_STREAM, 0);
   if (listen_sd < 0)
   {
      perror("socket() failed");
      exit(-1);
   }
   return 0;
}
int server_class::set_socket_non_blocking()
{
   /*************************************************************/
   /* Set socket to be nonblocking. All of the sockets for      */
   /* the incoming connections will also be nonblocking since   */
   /* they will inherit that state from the listening socket.   */
   /*************************************************************/
   rc = ioctl(listen_sd, FIONBIO, (char *)&on);
   if (rc < 0)
   {
      perror("ioctl() failed");
      close(listen_sd);
      exit(-1);
   }

   /*************************************************************/
   /* Allow socket descriptor to be reuseable                   */
   /*************************************************************/
   rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                   (char *)&on, sizeof(on));
   if (rc < 0)
   {
      perror("setsockopt() failed");
      close(listen_sd);
      exit(-1);
   }

   return 0;
}
int server_class::bind_socket()
{
   /*************************************************************/
   /* Bind the socket                                           */
   /*************************************************************/
   memset(&addr, 0, sizeof(addr));
   addr.sin6_family      = AF_INET6;
   memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
   addr.sin6_port        = htons(PORT);
   rc = bind(listen_sd,
             (struct sockaddr *)&addr, sizeof(addr));
   if (rc < 0)
   {
      perror("bind() failed");
      close(listen_sd);
      exit(-1);
   }
   return 0;
}
int server_class::listen_back_log()
{
   /* Set the listen back log                                   */
   rc = listen(listen_sd, 32);
   if (rc < 0)
   {
      perror("listen() failed");
      close(listen_sd);
      exit(-1);
   }
   return 0;
}
int server_class::init_fd_sets()
{
   FD_ZERO(&master_set);
   max_sd = listen_sd;
   FD_SET(listen_sd, &master_set);
   return 0;
}
int server_class::set_timeout(int secs)
{
   /*************************************************************/
   /* Initialize the timeval struct to 3 minutes.  If no        */
   /* activity after 3 minutes this program will end.           */
   /*************************************************************/
   timeout.tv_sec  = secs;
   timeout.tv_usec = 0;

   return 0;
}
//returns -1 on error
int server_class::get_ready_incoming()
{
    //accept all listening client sockets
      
      /**********************************************************/
      /* Copy the master fd_set over to the working fd_set.     */
      /**********************************************************/
      memcpy(&working_set, &master_set, sizeof(master_set));

      /**********************************************************/
      /* Call select() and wait 3 minutes for it to complete.   */
      /**********************************************************/
      //printf("Waiting on select()...\n");
      rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

      /**********************************************************/
      /* Check to see if the select call failed.                */
      /**********************************************************/
      if (rc < 0)
      {
         perror("  select() failed");
         return -1;
      }

      /**********************************************************/
      /* Check to see if the 3 minute time out expired.         */
      /**********************************************************/
      if (rc == 0)
      {
         printf("  select() timed out.  End program.\n");
         return -1;
      }
      return 0;
}
int server_class::clean_connection(int fd)
{
   /*************************************************/
   /* If the close_conn flag was turned on, we need */
   /* to clean up this active connection.  This     */
   /* clean up process includes removing the        */
   /* descriptor from the master set and            */
   /* determining the new maximum descriptor value  */
   /* based on the bits that are still turned on in */
   /* the master set.                               */
   /*************************************************/
   if (close_conn)
   {
      close(i);
      FD_CLR(i, &master_set);
      if (i == max_sd)
      {
         while (FD_ISSET(max_sd, &master_set) == FALSE)
            max_sd -= 1;
      }
   }
   return 0;
}
int server_class::receive_message()
{
   do
   {
   /**********************************************/
   /* Receive data on this connection until the  */
   /* recv fails with EWOULDBLOCK.  If any other */
   /* failure occurs, we will close the          */
   /* connection.                                */
   /**********************************************/
   memset(buffer, '\0', sizeof(char)*80);
   rc = recv(i, buffer, sizeof(buffer), 0);
   if (rc < 0)
   {
      if (errno != EWOULDBLOCK)
      {
         perror("  recv() failed");
         close_conn = TRUE;
      }
      break;
   }

   /**********************************************/
   /* Check to see if the connection has been    */
   /* closed by the client                       */
   /**********************************************/
   if (rc == 0)
   {
      printf("  Connection closed\n");
      close_conn = TRUE;
      break;
   }

   /**********************************************/
   /* Data was received                          */
   /**********************************************/
   len = rc;
   printf("  %d bytes received\n", len);
   for (int k=0; k<80; k++) printf("%c",buffer[k]);
   printf("\n");
   /**********************************************/
   /* Echo the data back to the client           */
   /**********************************************/
   rc = send(i, buffer, len, 0);
   if (rc < 0)
   {
      perror("  send() failed");
      close_conn = TRUE;
      break;
   }

   } while (TRUE);
   return 0;
}
server_class::~server_class()
{
   for (i=0; i <= max_sd; ++i)
   {
      if (FD_ISSET(i, &master_set))
         close(i);
   }
}
int main()
{
   server_class server(12345);
   server.create_socket();
   server.set_socket_non_blocking();
   server.bind_socket();
   server.listen_back_log();
   server.init_fd_sets();
   server.set_timeout(3600);
   do{
      if (server.get_ready_incoming() == -1) 
         break;
      server.desc_ready = server.rc;
      for (int i=0; i <= server.max_sd  &&  server.desc_ready > 0; ++i)
      {
         /*******************************************************/
         /* Check to see if this descriptor is ready            */
         /*******************************************************/
         if (FD_ISSET(i, &server.working_set))
         {
            /****************************************************/
            /* A descriptor was found that was readable - one   */
            /* less has to be looked for.  This is being done   */
            /* so that we can stop looking at the working set   */
            /* once we have found all of the descriptors that   */
            /* were ready.                                      */
            /****************************************************/
            server.desc_ready -= 1;

            /****************************************************/
            /* Check to see if this is the listening socket     */
            /****************************************************/
            if (i == server.listen_sd)
            {
               printf("  Listening socket is readable\n");
               /*************************************************/
               /* Accept all incoming connections that are      */
               /* queued up on the listening socket before we   */
               /* loop back and call select again.              */
               /*************************************************/
               //server.receive_message();
                /*************************************************/
               /* Accept all incoming connections that are      */
               /* queued up on the listening socket before we   */
               /* loop back and call select again.              */
               /*************************************************/
               //server.receive_message();
               do
               {
                  /**********************************************/
                  /* Accept each incoming connection.  If       */
                  /* accept fails with EWOULDBLOCK, then we     */
                  /* have accepted all of them.  Any other      */
                  /* failure on accept will cause us to end the */
                  /* server.                                    */
                  /**********************************************/
                  server.new_sd = accept(server.listen_sd, NULL, NULL);
                  if (server.new_sd < 0)
                  {
                     if (errno != EWOULDBLOCK)
                     {
                        perror("  accept() failed");
                        server.end_server = TRUE;
                     }
                     break;
                  }

                  /**********************************************/
                  /* Add the new incoming connection to the     */
                  /* master read set                            */
                  /**********************************************/
                  printf("  New incoming connection - %d\n", server.new_sd);
                  FD_SET(server.new_sd, &server.master_set);
                  if (server.new_sd > server.max_sd)
                     server.max_sd = server.new_sd;

                  /**********************************************/
                  /* Loop back up and accept another incoming   */
                  /* connection                                 */
                  /**********************************************/
               } while (server.new_sd != -1);
            }

            /****************************************************/
            /* This is not the listening socket, therefore an   */
            /* existing connection must be readable             */
            /****************************************************/
            else
            {
               printf("  Descriptor %d is readable\n", i);
               server.close_conn = FALSE;
               /*************************************************/
               /* Receive all incoming data on this socket      */
               /* before we loop back and call select again.    */
               /*************************************************/
               do
               {
                  /**********************************************/
                  /* Receive data on this connection until the  */
                  /* recv fails with EWOULDBLOCK.  If any other */
                  /* failure occurs, we will close the          */
                  /* connection.                                */
                  /**********************************************/
                  memset(server.buffer, '\0', sizeof(char)*80);
                  server.rc = recv(i, server.buffer, sizeof(server.buffer), 0);
                  if (server.rc < 0)
                  {
                     if (errno != EWOULDBLOCK)
                     {
                        perror("  recv() failed");
                        server.close_conn = TRUE;
                     }
                     break;
                  }

                  /**********************************************/
                  /* Check to see if the connection has been    */
                  /* closed by the client                       */
                  /**********************************************/
                  if (server.rc == 0)
                  {
                     printf("  Connection closed\n");
                     server.close_conn = TRUE;
                     break;
                  }

                  /**********************************************/
                  /* Data was received                          */
                  /**********************************************/
                  server.len = server.rc;
                  //printf("  %d bytes received\n", server.len);
                  for (int k=0; k<80; k++) printf("%c",server.buffer[k]);
                  printf("\n");
                  /**********************************************/
                  /* Echo the data back to the client           */
                  /**********************************************/
                  server.rc = send(i, server.buffer, server.len, 0);
                  if (server.rc < 0)
                  {
                     perror("  send() failed");
                     server.close_conn = TRUE;
                     break;
                  }

               } while (TRUE);

               /*************************************************/
               /* If the close_conn flag was turned on, we need */
               /* to clean up this active connection.  This     */
               /* clean up process includes removing the        */
               /* descriptor from the master set and            */
               /* determining the new maximum descriptor value  */
               /* based on the bits that are still turned on in */
               /* the master set.                               */
               /*************************************************/
               if (server.close_conn)
               {
                  close(i);
                  FD_CLR(i, &server.master_set);
                  if (i == server.max_sd)
                  {
                     while (FD_ISSET(server.max_sd, &server.master_set) == FALSE)
                        server.max_sd -= 1;
                  }
               }
            } /* End of existing connection is readable */
            
         } /* End of if (FD_ISSET(i, &working_set)) */
      } /* End of loop through selectable descriptors */

   } while (server.end_server == FALSE);

   /*************************************************************/
   /* Clean up all of the sockets that are open                 */
   /*************************************************************/
   // for (i=0; i <= max_sd; ++i)
   // {
   //    if (FD_ISSET(i, &master_set))
   //       close(i);
   // }
   // return 0;
            
         
      
   return 0;
}
