#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

#define PORT 8080

int main (void)
{
  int sd;
  struct sockaddr_in server;
  int nr=0;
  char msg[1024];
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons (PORT);
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("Connection Error");
      return errno;
    }
  printf ("[client]Introduceti path-ul fisierului pe care doriti sa il incarcati: ");
  fflush (stdout);
  read (0, msg, sizeof(msg));
  printf("[client] Se trimite fisierul %s\n",msg);
  fflush(stdout);
  if (write (sd,&msg,sizeof(msg)) <= 0)
    {
      perror ("[client] Writing Error\n");
      return errno;
    }
  if (read (sd, &msg,sizeof(msg)) < 0)
    {
      perror ("[client]Reading Error.\n");
      return errno;
    }
  printf ("[client]Mesajul primit este: %s\n", msg);
  close (sd);
}