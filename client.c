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
        perror("Socket Error().\n");
        return errno;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);
    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("Connection Error");
        return errno;
    }
    char rsp1[1025];
    while(1) {
        int ln = 0;
        read(sd, &ln, sizeof(ln));
        ln = ntohl(ln);
        if(ln == 0) break;
        read(sd, rsp1, ln);
        rsp1[ln] = '\0';
        printf("%s", rsp1);
    }
    fflush(stdout);
    FILE *srcFile = NULL;
    char *realmsg = NULL;
    while (srcFile == NULL) {
        printf("[client]Introduceti path-ul fisierului pe care doriti sa il incarcati: ");
        fflush(stdout);
        bzero(msg, sizeof(msg));
        if (realmsg != NULL) bzero(realmsg, sizeof(realmsg));
        read(0, msg, sizeof(msg));
        msg[strlen(msg) - 1] = '\0';
        fflush(stdout);
        realmsg = realpath(msg, NULL);
        if(realmsg != NULL) {
            char *extension = strrchr(realmsg, '.');
            if (extension != NULL && strcmp(extension, ".c") == 0 || strcmp(extension, ".cpp") == 0) {
                srcFile = fopen(realmsg, "rb");
            }
        }
    }
    printf("[client] Se trimite fisierul %s\n", realmsg);
    fflush(stdout);
    char buffer[512] = {0};
    bzero(buffer, sizeof(buffer));
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
        int sent_length = htonl(bytes_read);
        write(sd, &sent_length, sizeof(sent_length));
        write(sd, buffer, bytes_read);
        bzero(buffer, sizeof(buffer));
    }
    bytes_read = htonl(bytes_read);
    write(sd, &bytes_read, sizeof(bytes_read));
    char response[1024] = {0};
    bzero(response, sizeof(response));
    int val = 0;
    read(sd, &val, sizeof(val));
    val = ntohl(val);
    if (read(sd, &response, val) < 0) {
        perror("[client]Reading Error.\n");
        return errno;
    }
    fflush(stdout);
    printf("[client]Mesajul primit este: %s\n", response);
    fflush(stdout);
    int val2 = 0;
    read(sd, &val2, sizeof(val2));
    val2 = ntohl(val2);
    char standing[1000] = {0};
    if(read(sd, &standing, val2) <= 0) {
        return errno;
    }
    standing[val2 + 1] = '\0';
    printf("%s\n", standing);
    close(sd);
}
