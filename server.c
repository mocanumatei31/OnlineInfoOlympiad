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
#include <sys/stat.h>

#define PORT 8080

extern int errno;

typedef struct thData {
    int idThread;
    int cl;
} thData;

void *client_handler(void *);

int main() {
    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;
    pthread_t th[100];
    int i = 0;
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
    while (1) {
        int client;
        int length = sizeof (from);
        printf("[server]Asteptam la portul %d...\n",PORT);
        fflush(stdout);
        if ((client = accept(sd, (struct sockaddr *) &from, &length)) < 0) {
            perror("[server]Eroare la accept().\n");
            continue;
        }
        thData *td = malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &client_handler, td);
    }
};

void *client_handler(void *arg) {
    const char *directory_name = "UserSources\0";
    struct thData tdL;
    tdL = *((struct thData *) arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    struct stat st = {0};
    if (stat(directory_name, &st) == -1) {
        if (mkdir(directory_name, 0777) == -1) {
            perror("[server]Eroare la mkdir().\n");
            return errno;
        }
    }
    char temp[100];
    sprintf(temp, "%s/source_c%d.c", directory_name, tdL.idThread);
    fflush(stdout);
    FILE *dstFile = fopen(temp, "wb");
    char msg[512];
    int valread;
    while (1) {
        int tmp;
        read(tdL.cl, &tmp, sizeof(tmp));
        valread = ntohl(tmp);
        if (valread == 0) {
            break;
        }
        read(tdL.cl, &msg, valread);
        fwrite(msg, 1, valread, dstFile);
        bzero(msg, sizeof(msg));
        fflush(dstFile);
    }
    printf("[Thread %d]Mesajul a fost receptionat...\n", tdL.idThread);
    printf("[Thread %d]Trimitem mesajul inapoi...\n", tdL.idThread);
    bzero(msg, 512);
    fclose(dstFile);
    char message[100] = "File Received!\n\0";
    valread = htonl(strlen(message));
    write(tdL.cl, &valread, sizeof(valread));
    if (write(tdL.cl, &message, strlen(message)) <= 0) {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    } else {
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    close((intptr_t) arg);
    return (NULL);
};
