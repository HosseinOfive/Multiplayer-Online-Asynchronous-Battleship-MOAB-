#ifndef PLAYER_H
#define PLAYER_H

#define MAX_STR_LEN 101

typedef struct player_node {
    int lives_left; //if gets to zero you lost
    char name[21];  // 20 chars + null terminator
    int ship[5][2];  //location of everypiece of sihp
    int fd; 
    int missed_reads;  // counts how many times client read was empty before registration
    struct player_node* next;  //its an ll
} Player;

Player *broadcast(Player *head, char *msg, int msg_size, int epfd);
Player *del_player(Player *head, int fd, int epfd);
Player *create_node(char name[21], int loc[5][2], int fd);
int is_registered(int fd, Player *head);
int unique_name(char *name, Player *head);
Player *got_hit(Player *head, int cord[2], char *name, int epfd);
Player *fd_to_player(Player *head, int fd);
void free_player(Player *head);
int broadcast_helper(int fd, char *msg, int msg_size);
Player *del_player_silent(Player *head, int fd, int epfd);

#endif
