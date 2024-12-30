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
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 8080

extern int errno;

char *conv_addr(struct sockaddr_in address) {
    static char str[25];
    char port[7];
    strcpy(str, inet_ntoa(address.sin_addr));
    bzero(port, 7);
    sprintf(port, ":%d", ntohs(address.sin_port));
    strcat(str, port);
    return (str);
}

typedef struct Contestant {
    int id;
    int fd;
    int isActive;
    int score;
} thData;

struct Contestant contestants[100];
int contestants_size;

int contestants_scores[1000];
int sent_message[1000];
int score[1000];
int has_contest_started = 0;
int problem_number = 0;

int client_handler(struct Contestant contestant);
int run_solution(int id);

int findContestant(int fd) {
    for (int i = 0; i < contestants_size; i++) {
        if (contestants[i].fd == fd && contestants[i].isActive) {
            return i;
        }
    }
}

char* generate_standing() {
    int max_length = 100 * contestants_size;
    char* standing = (char *)malloc(max_length);
    bzero(standing, max_length);
    if(standing == NULL) {
        perror("malloc");
        exit(1);
    }
    for(int i = 0; i < contestants_size; i++) {
        char player_string[100];
        sprintf(player_string, "%d. Concurent %d - %d Puncte\n\0", i, contestants[i].id, contestants[i].score);
        strcat(standing, player_string);
    }
    standing[strlen(standing) - 1] = '\0';
    return standing;
}

int main() {
    struct sockaddr_in server;
    struct sockaddr_in from;
    fd_set readfds;
    fd_set actfds;
    struct timeval tv;
    int sd;
    int optval = 1;
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
    //srand(time(NULL));
    //problem_number = rand() % 5 + 1;
    problem_number = 1;
    FD_ZERO(&actfds);
    FD_SET(sd, &actfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    time_t start_time = time(NULL);
    int nfds = sd;
    while (1) {
        if(!has_contest_started) {
            has_contest_started = 1;
            start_time = time(NULL);
        }
        if(has_contest_started) {
            time_t current_time = time(NULL);
            if(difftime(current_time, start_time) > 30) {
                break;
            }
        }
        bcopy((char *) &actfds, (char *) &readfds, sizeof (readfds));
        if (select(nfds + 1, &readfds, NULL, NULL, &tv) < 0) {
            perror("[server] Eroare la select().\n");
            return errno;
        }
        if (FD_ISSET(sd, &readfds)) {
            printf("7\n");
            len = sizeof (from);
            bzero(&from, sizeof (from));
            int client = accept(sd, (struct sockaddr *) &from, &len);
            if (client < 0) {
                perror("[server] Eroare la accept().\n");
                continue;
            }

            if (nfds < client)
                nfds = client;
            FD_SET(client, &actfds);
            if(contestants_size < 3) {
                printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n", client, conv_addr(from));
                struct Contestant contestant;
                contestant.fd = client;
                contestant.id = id_cnt++;
                contestant.isActive = 1;
                contestant.score = 0;
                contestants[contestants_size++] = contestant;
                fflush(stdout);
            } else {
                close(client);
                FD_CLR(client, &actfds);
                printf("Client %d respins\n", contestants_size);
            }
        }
        if(contestants_size >= 3) {
            for (int fd = 0; fd <= nfds; fd++) {
                if (fd != sd && FD_ISSET(fd, &readfds) && sent_message[fd]) {
                    int contId = findContestant(fd);
                    struct Contestant contestant = contestants[contId];
                    if (client_handler(contestant)) {
                        if (contestants[contId].isActive) {
                            run_solution(contestants[contId].id);
                        }
                    }
                }
                else if(fd != sd && FD_ISSET(fd, &actfds) && !sent_message[fd]) {
                    char* message = "Started\n\0";
                    int x = htonl(strlen(message));
                    write(fd, &x, sizeof(x));
                    write(fd, message, strlen(message));
                    sent_message[fd] = 1;
                }
            }
        }
    }
    printf("Reached\n");
    char* standing = {0};
    standing = generate_standing();
    standing[strlen(standing)] = '\0';
    int snd_len = strlen(standing);
    int stand_len = htonl(snd_len);
    bcopy((char *) &actfds, (char *) &readfds, sizeof (readfds));
    if (select(nfds + 1, &readfds, NULL, NULL, &tv) < 0) {
        perror("[server] Eroare la select().\n");
        return errno;
    }
    for (int fd = 0; fd <= nfds; fd++) {
        if (fd != sd && FD_ISSET(fd, &actfds) && sent_message[fd]) {
            int contId = findContestant(fd);
            write(fd, &stand_len, sizeof(stand_len));
            if(write(fd, standing, strlen(standing)) <= 0) {
                perror("[server] Eroare la write.\n");
                return errno;
            }
            printf("[server] S-a deconectat clientul cu descriptorul %d.\n", fd);
            fflush(stdout);
            contestants[contId].isActive = 0;
            close(fd);
            FD_CLR(fd, &actfds);
        }
    }
    free(standing);
    return 0;

}

int compare_output(int test) {
    char outputfile[1000] = {0};
    sprintf(outputfile, "Tests/Problem%d/output%d.txt\0", problem_number, test);
    char expfile[1000] = {0};
    sprintf(expfile, "Tests/Problem%d/exp_output%d.txt\0", problem_number, test);
    FILE* fp = fopen(outputfile, "r");
    FILE* exp_fp = fopen(expfile, "r");
    if (fp == NULL || exp_fp == NULL) {
        perror("[server] Eroare la open.\n");
        return errno;
    }
    char buf1[1024];
    char buf2[1024];
    while(fgets(buf1, sizeof(buf1), fp) && fgets(buf2, sizeof(buf2), exp_fp)) {
        if(strcmp(buf1, buf2) != 0) {
            printf("%s %s\n",buf1, buf2);
            fclose(fp);
            fclose(exp_fp);
            return 0;
        }
    }
    if(fgets(buf1, sizeof(buf1), fp) || fgets(buf2, sizeof(buf2), exp_fp)) {
        fclose(fp);
        fclose(exp_fp);
        return 0;
    }
    fclose(fp);
    fclose(exp_fp);
    return 1;
}

int run_solution(int id) {
    const char *directory_name = "UserSources\0";
    char temp[100];
    sprintf(temp, "%s/source_c%d.c", directory_name, id);
    char command[300];
    char ex_name[150];
    sprintf(ex_name, "source_c%d", id);
    sprintf(command, "gcc %s -o %s", temp, ex_name);
    sprintf(ex_name, "./source_c%d", id);
    int result = system(command);
    //if(result != 0) {
    //    contestants[id].score = 0;
    //    return 0;
    //}
    for(int i = 1; i <= 5; i++) {
        int pid = fork();
        if(pid == 0) {
            char testfile[1000] = {0};
            sprintf(testfile, "Tests/Problem%d/test%d.txt\0", problem_number, i);
            int fd = open(testfile, O_RDONLY);
            char outputfile[1000] = {0};
            sprintf(outputfile, "Tests/Problem%d/output%d.txt\0", problem_number, i);
            int fd_out = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(fd < 0) {
                perror("[server] Eroare la open().\n");
                return errno;
            }
            if(dup2(fd, STDIN_FILENO) < 0) {
                perror("[server] Eroare la dup2().\n");
                return errno;
            }
            if(fd_out < 0) {
                perror("[server] Eroare la open().\n");
                return errno;
            }
            if(dup2(fd_out, STDOUT_FILENO) < 0) {
                perror("[server] Eroare la dup2().\n");
                return errno;
            }
            close(fd);
            close(fd_out);
            char* args[] = {ex_name, NULL};
            execv(ex_name, args);
            exit(1);
        }
        else {
            int status;
            waitpid(pid, &status, 0);
            int test_res = compare_output(i);
            if(test_res == 1) contestants[id].score += 20;
        }
    }
    //contestants[id].score = result == 0 ? 100 : 0;
    return 1;
}

int client_handler(struct Contestant contestant) {
    const char *directory_name = "UserSources\0";
    printf("[Concurent]- %d - Asteptam mesajul...\n", contestant.id);
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
        if (read(contestant.fd, &tmp, sizeof(tmp)) <= 0) {
            return 1;
        }
        valread = ntohl(tmp);
        if (valread == 0) {
            break;
        }
        if (read(contestant.fd, &msg, valread) <= 0) {
            return 1;
        }
        fwrite(msg, 1, valread, dstFile);
        bzero(msg, sizeof(msg));
        fflush(dstFile);
    }
    printf("[Concurent %d]Mesajul a fost receptionat...\n", contestant.id);
    printf("[Concurent %d]Trimitem mesajul inapoi...\n", contestant.id);
    bzero(msg, 512);
    fclose(dstFile);
    char message[100] = "File Received!\n\0";
    valread = htonl(strlen(message));
    if (write(contestant.fd, &valread, sizeof(valread)) <= 0) {
        printf("[Concurent %d] ", contestant.id);
        perror("[Concurent]Eroare la write() catre client.\n");
    }
    if (write(contestant.fd, &message, strlen(message)) <= 0) {
        printf("[Concurent %d] ", contestant.id);
        perror("[Concurent]Eroare la write() catre client.\n");
    }
    else {
        printf("[Concurent %d]Mesajul a fost trasmis cu succes.\n", contestant.id);
    }
    return 1;
};