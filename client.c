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

extern int errno;

#define PORT 8080

int main(void) {
    int sd;
    struct sockaddr_in server;
    int nr = 0;
    char msg[1024];
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Eroare la socket().\n");
        return errno;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);
    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("Connection Error");
        return errno;
    }
    FILE* srcFile = NULL;
    char* realmsg = NULL;
    while(srcFile == NULL) {
        printf("[client]Introduceti path-ul fisierului pe care doriti sa il incarcati: ");
        fflush(stdout);
        bzero(msg, sizeof(msg));
        if(realmsg != NULL) bzero(realmsg, sizeof(realmsg));
        read(0, msg, sizeof(msg));
        msg[strlen(msg) - 1] = '\0';
        fflush(stdout);
        realmsg = realpath(msg, NULL);
        srcFile = fopen(realmsg, "rb");
    }
    printf("[client] Se trimite fisierul %s\n", realmsg);
    fflush(stdout);
    if(srcFile == NULL) return 0;
    char buffer[512] = {0};
    bzero(buffer, sizeof(buffer));
    int bytes_read;
    while((bytes_read = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
        printf("%d\n", bytes_read);
        int sent_length = htonl(bytes_read);
        write(sd, &sent_length, sizeof(sent_length));
        write(sd, buffer, bytes_read);
        bzero(buffer, sizeof(buffer));
    }
    printf("%d\n",bytes_read);
    bytes_read = htonl(bytes_read);
    //1610612736
    printf("%d\n", bytes_read);
    write(sd, &bytes_read, sizeof(bytes_read));
    printf("5\n");
    char response[1024];
    if (read(sd, &response, sizeof(response)) < 0) {
        perror("[client]Reading Error.\n");
        return errno;
    }
    printf("[client]Mesajul primit este: %s\n", response);
    close(sd);
}
