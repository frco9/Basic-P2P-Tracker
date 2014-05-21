#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include "queue.h"
#include <unistd.h>
#include <pthread.h> 

#define __BUFF_SIZE__ 2048
#define __CLOSE_MESSAGE__ "close"
#define TTL 150
#define SEED 0
#define LEECH 1
#define UNDEF 2

struct active_connection{
    int sock;
    char* ip;
    int sys_port;
    TAILQ_ENTRY(active_connection) next;
};

typedef struct active_connection* active_connection;
TAILQ_HEAD(active_connections_head, active_connection) active_connections;

struct file_s{
  char* key;
  char* name;
  long t_length;
  long p_length;
  int ttl;
  int sl;
  //int numP; //nombre de pairs qui possèdent le fichier
  TAILQ_ENTRY(file_s) next;
  TAILQ_ENTRY(file_s) nextAll;
  //TAILQ_HEAD(client_list_head, client_s) client_list;
};

struct client_s{
  char* ip;
  int port;
  int sys_port;
  TAILQ_ENTRY(client_s) next;
  TAILQ_HEAD(file_list_head, file_s) file_list;
};

TAILQ_HEAD(file_list_head1, file_s) file_list_all;
TAILQ_HEAD(client_list_head, client_s) client_list;

typedef struct file_s* file_s;
typedef struct client_s* client_s;



