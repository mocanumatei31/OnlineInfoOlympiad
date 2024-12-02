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

#define PORT 8080

extern int errno;

typedef struct thData {
    int idThread;
    int cl;
} thData;

void *treat(void *);

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

        pthread_create(&th[i], NULL, &treat, td);
    }
};

void *treat(void *arg) {
    struct thData tdL;
    tdL = *((struct thData *) arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    char msg[1024];
    if (read(tdL.cl, &msg, sizeof(msg)) <= 0) {
        printf("[Thread %d]\n", tdL.idThread);
        perror("Reading Error.\n");
    }
    printf("[Thread %d]Mesajul a fost receptionat...%s\n", tdL.idThread, msg);
    printf("[Thread %d]Trimitem mesajul inapoi...%s\n", tdL.idThread, msg);
    if (write(tdL.cl, &msg, sizeof(msg)) <= 0) {
        printf("[Thread %d] ", tdL.idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }
    else {
        printf("[Thread %d]Mesajul a fost trasmis cu succes.\n", tdL.idThread);
    }
    close((intptr_t) arg);
    return (NULL);
};

