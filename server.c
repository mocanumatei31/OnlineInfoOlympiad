#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080

extern int errno;

char * conv_addr (struct sockaddr_in address) {
    static char str[25];
    char port[7];
    strcpy (str, inet_ntoa (address.sin_addr));
    bzero (port, 7);
    sprintf (port, ":%d", ntohs (address.sin_port));
    strcat (str, port);
    return (str);
}

typedef struct Contestant {
    int id;
    int fd;
} thData;

struct Contestant contestants[100];
int contestants_size;

int client_handler(struct Contestant contestant);

struct Contestant findContestant(int fd) {
    for (int i = 0; i < contestants_size; i++) {
        if (contestants[i].fd == fd) {
            return contestants[i];
        }
    }
}


int main() {
    struct sockaddr_in server;
    struct sockaddr_in from;
    fd_set readfds;
    fd_set actfds;
    struct timeval tv;
    int sd;
    int optval=1;
    int len;
    int i = 0;
    int id_cnt = 0;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    int on = 1;
    setsockopt(sd,SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));
    bzero(&server, sizeof (server));
    bzero(&from, sizeof (from));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 2) == -1) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }
    FD_ZERO (&actfds);
    FD_SET (sd, &actfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int nfds = sd;
    while (1) {
        bcopy ((char *) &actfds, (char *) &readfds, sizeof (readfds));
        if (select (nfds+1, &readfds, NULL, NULL, &tv) < 0)
        {
            perror ("[server] Eroare la select().\n");
            return errno;
        }
        if (FD_ISSET (sd, &readfds))
        {
            len = sizeof (from);
            bzero (&from, sizeof (from));
            int client = accept(sd, (struct sockaddr *) &from, &len);
            if (client < 0)
            {
                perror ("[server] Eroare la accept().\n");
                continue;
            }

            if (nfds < client)
                nfds = client;
            FD_SET (client, &actfds);
            printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n",client, conv_addr (from));
            struct Contestant contestant;
            contestant.fd = client;
            contestant.id = id_cnt++;
            contestants[contestants_size++] = contestant;
            fflush (stdout);
        }
        for (int fd = 0; fd <= nfds; fd++)
        {
            if (fd != sd && FD_ISSET (fd, &readfds))
            {
                struct Contestant contestant = findContestant(fd);
                if (client_handler(contestant))
                {
                    printf ("[server] S-a deconectat clientul cu descriptorul %d.\n",fd);
                    fflush (stdout);
                    close (fd);
                    FD_CLR (fd, &actfds);
                }
            }
        }
    }
};

int client_handler(struct Contestant contestant) {
    const char *directory_name = "UserSources\0";
    printf("[thread]- %d - Asteptam mesajul...\n", contestant.id);
    struct stat st = {0};
    if (stat(directory_name, &st) == -1) {
        if (mkdir(directory_name, 0777) == -1) {
            perror("[server]Eroare la mkdir().\n");
            return errno;
        }
    }
    char temp[100];
    sprintf(temp, "%s/source_c%d.c", directory_name, contestant.id);
    fflush(stdout);
    FILE *dstFile = fopen(temp, "wb");
    char msg[512];
    int valread;
    while (1) {
        int tmp;
        read(contestant.fd, &tmp, sizeof(tmp));
        valread = ntohl(tmp);
        if (valread == 0) {
            break;
        }
        read(contestant.fd, &msg, valread);
        fwrite(msg, 1, valread, dstFile);
        bzero(msg, sizeof(msg));
        fflush(dstFile);
    }
    printf("[Thread %d]Mesajul a fost receptionat...\n", contestant.id);
    printf("[Thread %d]Trimitem mesajul inapoi...\n", contestant.id);
    bzero(msg, 512);
    fclose(dstFile);
    char message[100] = "File Received!\n\0";
    valread = htonl(strlen(message));
    write(contestant.fd, &valread, sizeof(valread));
    if (write(contestant.fd, &message, strlen(message)) <= 0) {
        printf("[Thread %d] ", contestant.id);
        perror("[Thread]Eroare la write() catre client.\n");
    } else {
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", contestant.id);
    }
    return 1;
};
