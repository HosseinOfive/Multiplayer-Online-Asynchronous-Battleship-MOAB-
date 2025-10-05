#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "player.h"
#define CLIENT_S 10
#define MAX_EVENTS 64 


int main(int argc,  char *argv[] ) {
    if( argc != 2) return 1; //too many/little arg
    int sfd, epfd;
    int port = atoi(argv[1]);
    struct sockaddr_in addr;
    struct epoll_event e;
    Player *head, *tail;
    head = NULL; 
    tail = NULL;

    // Ignore SIGPIPE so write errors don't stop the server
    struct sigaction a;
    a.sa_handler = SIG_IGN;
    sigfillset(&a.sa_mask);
    a.sa_flags = 0;
    sigaction(SIGPIPE, &a, NULL);
    char buffer[MAX_STR_LEN];

    // Create socket
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sfd);
        return 1;
    }
    if (listen(sfd, CLIENT_S) == -1) {
        perror("listen");
        close(sfd);
        return 1;
    }

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        close(sfd);
        return 1;
    }

    e.events = EPOLLIN;
    e.data.fd = sfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &e) == -1) {
        perror("listen_socket");
        close(sfd);
        close(epfd);
        return 1;
    }

    for (;;) {
        struct epoll_event events[CLIENT_S];
        int i, n, cfd;
        n = epoll_wait(epfd, events, CLIENT_S, -1);
        /*
        if (n == -1) {
            if (errno == EINTR) continue; // interrupted by signal, retry
            perror("epoll_wait");
            break;
        }
        */ 
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == sfd) {  //a new client
                int cfd = accept(sfd, NULL, NULL);
                if (cfd == -1) {
                    perror("accept");
                    continue;
                }
                e.events = EPOLLIN;
                e.data.fd = cfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &e) == -1) {
                    perror("cfd");
                    close(cfd);
                    continue;
                }
            } else { //the existing guy
                
                int cfd = events[i].data.fd;
                Player *player = fd_to_player(head, cfd);

                if (player == NULL) {   //he is not in the LL(need to register him)
                    
                    int flag = read(cfd, buffer, sizeof(buffer) - 1);
                    if (flag < 1) {
                        // Client closed or error: just close fd and remove from epoll
                        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                        close(cfd);
                        continue; //jst mean do nothing with this guy from now on
                    }
                    buffer[flag] = '\0';

                    Player temp_player = {0};
                    char name[21];
                    int x, y;
                    char d;
                    int scan_flag = 0;

                    if (4 != sscanf(buffer, "REG %20s %d %d %c\n", name, &x, &y, &d))   scan_flag = -1;
                    else if( d == '-' && (2 > x || x > 7)) scan_flag = -1;
                    else if( d == '|' && (2 > y || y > 7)) scan_flag = -1;

                    if (scan_flag == -1) {
                        if (write(cfd, "INVALID\n", 8) < 0) {
                            epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                            close(cfd);
                        }
                        continue;
                    }

                    if (unique_name(name, head)) {
                        if (write(cfd, "TAKEN\n", 6) < 0) {
                            epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                            close(cfd);
                        }
                        continue;
                    }

                    int loc[5][2] = {0};
                    /*
                    int j = (d == '-') ? 0 : 1;
                    for (int k = 0; k < 5; k++) {
                        loc[k][0] = x;
                        loc[k][1] = y;
                        loc[k][j] = loc[k][j] - 2 + k;
                    }
                    */
                    int j;
                    if( d == '-') j = 0;
                    else j = 1;
                    for(int i = 0; i < 5; i++){
                        loc[i][0] = x; //these are where the ship stands ///////////THIS IS WRONG
                        loc[i][1] = y;
                        loc[i][j] = loc[i][j] -2 +i;
                    }

                    if (write(cfd, "WELCOME\n", 8) < 0) {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                        close(cfd);
                        continue;
                    }

                    Player *node; 
                    node = create_node(name, loc, cfd);
                    
                    /*if (node == NULL) {
                        fprintf(stderr, "Failed to allocate player node\n");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                        close(cfd);
                        continue;
                    }
                    */
                    if (head == NULL)  head = tail = node;
                    else {
                        tail->next = node;
                        tail = node;
                    }
                    ///now we register the player
                    char msg[MAX_STR_LEN];
                    int written = snprintf(msg, sizeof(msg), "JOIN %s\n", name);
                    head = broadcast(head, msg, written, epfd);

                } else {
                    // now the client is registered and wants to bomb somewhere
                    int flag = read(cfd, buffer, sizeof(buffer) - 1);
                    if (flag  < 1) {
                        head = del_player(head, cfd, epfd);
                        continue;
                    }
                    buffer[flag] = '\0';

                    int cord[2];
                    int scan_flag = 0;

                    if (2 != sscanf(buffer, "BOMB %d %d\n", &cord[0], &cord[1])) {
                        scan_flag = -1;
                    }

                    if (scan_flag == -1) {
                        if (write(cfd, "INVALID\n", 8) < 0) {
                            head = del_player(head, cfd, epfd);
                        }
                        continue;
                    }

                    head = got_hit(head, cord, player->name, epfd);
                }
            }
        }
    }

    free_player(head);
    close(sfd);
    close(epfd);
    return 0;
}
