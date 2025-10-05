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
#define MAX_STR_LEN 101
#define MAX_MISSED_READS 5  // max allowed empty reads from unregistered clients

int broadcast_helper(int fd, char *msg, int msg_size) {
    int err = write(fd, msg, msg_size);
    if (err < 0) return -1;
    return 0;
}

Player *del_player_silent(Player *head, int fd, int epfd) {
    Player *q, *p;
    p = head;
    if (head == NULL) return NULL;
    //while(p->next != NULL){
    if (p->fd == fd) {
        q = p->next;
        ////////broadcast( head, buff, size);  the diffrence between silent and normal
        epoll_ctl(epfd, EPOLL_CTL_DEL, p->fd, NULL);
        close(p->fd);
        free(p);
        return q;
    }
    while (p->next != NULL) {
        if (p->next->fd == fd) {
            q = p->next->next;
            epoll_ctl(epfd, EPOLL_CTL_DEL, p->next->fd, NULL);
            close(p->next->fd);
            free(p->next);
            p->next = q;
            return head;
        }
        p = p->next;
    }
    return head;
}

Player *broadcast(Player *head, char *msg, int msg_size, int epfd) { //this broadcasts the message to all available players in game
    int arr[CLIENT_S] = { 0};
    int rem = 0;   
    Player *p = head;
    while (p != NULL) {
        if (broadcast_helper(p->fd, msg, msg_size) == -1) {
            arr[rem] = p->fd;   
            rem++;
        }
        p = p->next;
    }
    for (int i = 0; i < rem && i < CLIENT_S; i++) {  //make sure not gettin a seg fault
        Player *del = fd_to_player(head, arr[i]);
        if (del != NULL) { //didn't solve my problem already (in the lower loops)
            char name[21];
            char buff[MAX_STR_LEN];
            strncpy(name, del->name, 20);
            name[20] = '\0';
            head = del_player_silent(head, arr[i], epfd);
            int size = snprintf(buff, sizeof(buff), "GG %s\n", name);
            head = broadcast(head, buff, size, epfd);
        }
    }
    return head;
}

Player *del_player(Player *head, int fd, int epfd) {
    Player *q, *p;
    p = head;
    if (head == NULL) return NULL;
    ///while(p->next != NULL){
    if (p->fd == fd) {
        q = p->next;
        char buff[MAX_STR_LEN];
        int size = snprintf(buff, sizeof(buff), "GG %s\n", p->name);
        head = broadcast(head, buff, size, epfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, p->fd, NULL);
        close(p->fd);
        free(p);
        return q;
    }
    while (p->next != NULL) {
        if (p->next->fd == fd) {
            q = p->next->next;
            char buff[MAX_STR_LEN];
            int size = snprintf(buff, sizeof(buff), "GG %s\n", p->next->name);
            head = broadcast(head, buff, size, epfd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, p->next->fd, NULL);
            close(p->next->fd);
            free(p->next);
            p->next = q;
            return head;
        }
        p = p->next;
    }
    return head;
}
void handler(int sig){
    return;
}


Player *create_node(char name[21], int loc[5][2], int fd) { //makes a node for the player
    Player *node = (Player *)calloc(1, sizeof(Player));
    if (node == NULL) return NULL;
    node->lives_left = 5;
    name[20] = '\0';
    strncpy(node->name, name, 20);
    node->name[20] = '\0'; //just in case!
    node->fd = fd;
    memcpy(node->ship, loc, sizeof(node->ship));
    node->missed_reads = 0;
    node->next = NULL;
    return node;
}

int is_registered(int fd, Player *head) {
    while (head != NULL) {
        if (head->fd == fd) return 1;
        head = head->next;
    }
    return 0;
}

int unique_name(char *name, Player *head) {
    while (head != NULL) {
        if (strcmp(head->name, name) == 0) return 1;
        head = head->next;
    }
    return 0;
}

Player *got_hit(Player *head, int cord[2], char *name, int epfd) { ///returns head gets the head and name and cordinate of hit
    int flag = 0;
    if (head == NULL) return NULL;

    Player *head_const = head;
    while (head != NULL) {
        if (strcmp(head->name, name) != 0) { //I cant hit myself now can I
            for (int i = 0; i < 5; i++) {
                if (memcmp(head->ship[i], cord, 2 * sizeof(int)) == 0) {
                    flag = 1;
                    char buff[MAX_STR_LEN];
                    int size = snprintf(buff, sizeof(buff), "HIT %s %d %d %s\n", name, cord[0], cord[1], head->name);
                    head_const = broadcast(head_const, buff, size, epfd);
                    if (head->lives_left == 1) {
                        head_const = del_player(head_const, head->fd, epfd);
                    } 
                    else { head->lives_left--; }
                    break; ///we already got his ship
                }
            }
        }
        head = head->next;


    }
    if (flag == 0) {
        char buff[MAX_STR_LEN];
        int size = snprintf(buff, sizeof(buff), "MISS %s %d %d\n", name, cord[0], cord[1]);
        head_const = broadcast(head_const, buff, size, epfd);
    }
    return head_const;
}

Player *fd_to_player(Player *head, int fd) {
    while (head != NULL) {
        if (head->fd == fd) return head;
        head = head->next;
    }
    return NULL;
}

void free_player(Player *head) {
    Player *p;

    while (head != NULL) {
        p = head->next;
        close(head->fd);
        free(head);
        head = p;
    }
}
