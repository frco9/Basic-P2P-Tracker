#include "tracker.h"
#include "parse.h"

static int main_socket;


void intHandler() {
	active_connection n1, n2;
	n1 = TAILQ_FIRST(&active_connections);
	while (n1 != NULL) {
		n2 = TAILQ_NEXT(n1, next);
		close(n1->sock);
		//free(n1);
		n1 = n2;
	}
	
	client_s client, tempC;
	file_s file, tempF;
	TAILQ_FOREACH_SAFE(client, &client_list, next, tempC){
	  TAILQ_FOREACH_SAFE(file, &(client->file_list), next, tempF){
	    TAILQ_REMOVE(&(client->file_list), file, next);
	    free(file->name);
	    free(file->key);
	    free(file);
	  }
	  TAILQ_REMOVE(&client_list, client, next);
	  free(client->ip);	  
	  free(client);
	}

	/* TAILQ_FOREACH(file, &file_list_all, next){ */
	/*   TAILQ_REMOVE(&file_list_all, file, next); */
	/*   free(file->name); */
	/*   free(file->key); */
	/*   free(file); */
	/* } */
	close(main_socket);
	printf("\n");
	exit(0);
}

void clean_client(active_connection client_args){

  client_s tmp;
  client_s client = NULL;
  file_s file;
  file_s tempF, fileC, tempCF;
  int isStillFile;

  TAILQ_FOREACH(tmp, &client_list, next){
    if((strcmp(client_args->ip, tmp->ip)==0) && client_args->sys_port == tmp->sys_port){
      client = tmp;
      
      TAILQ_FOREACH_SAFE(file, &(client->file_list), next, tempF){
        TAILQ_REMOVE(&(client->file_list), file, next);
        isStillFile = 0;
        TAILQ_FOREACH(tmp, &client_list, next){
          if((strcmp(tmp->ip, client->ip)!=0) || tmp->sys_port != client->sys_port){
            TAILQ_FOREACH_SAFE(fileC, &(tmp->file_list), next, tempCF){
              if(strcmp(fileC->key, file->key) == 0){
                isStillFile = 1;
                break;
              }
            }
          }
          if(isStillFile == 1)
            break;
        }
        if(isStillFile == 0){
          TAILQ_REMOVE(&file_list_all, file, nextAll);
          free(file->name);
          free(file->key);
          free(file);
        }
      }
      break;
    }
  }
  if(client!=NULL){
    TAILQ_REMOVE(&client_list, client, next);
    free(client->ip);
    free(client);
  }
}

void *threadfunc(void *arg){
	active_connection socket_args = (active_connection) arg;
	int socket_fd = socket_args->sock;
	int read_size; 
	char message[__BUFF_SIZE__];
	//Wait&read message from client
	while((read_size=read(socket_fd , message , __BUFF_SIZE__ )) > 0){
		// Hack to get strcmp working
		if(read_size >= 1)
			message[read_size-1] = '\0';
		// Check if client send message to end connection
		if(strcmp(message, __CLOSE_MESSAGE__) == 0){
			printf("Client disconnected\n");
			char *confirmation = "Now disconnected";
			write(socket_fd , confirmation , strlen(confirmation));
			clean_client(socket_args);
			break;
		}
		
		// Restore EOL at the end of message to get it correctly displayed
		message[read_size-1] = '\n';
		message[read_size] = '\0';
		printf("Log from client %s : %s\n", socket_args->ip, message);
		init(socket_args, message);
		message[0] = '\0';

		client_s temp;
		file_s file;

		TAILQ_FOREACH(temp, &client_list, next){
		  printf("CLIENT : %s, %d, %d\n", temp->ip, temp->port, temp->sys_port);
		  if(!TAILQ_EMPTY(&(temp->file_list))){
		    //printf("salut\n");
			TAILQ_FOREACH(file, &(temp->file_list), next){
			  printf("FILE : %s, %s, %ld, %ld, %d, %d\n", file->key, file->name, file->t_length, file->p_length, file->ttl, file->sl);
			}
		      }
		}
		printf("\n\n");
		TAILQ_FOREACH(file, &file_list_all, nextAll){
		  printf("FILEALL : %s, %s, %ld, %ld, %d, %d\n", file->key, file->name, file->t_length, file->p_length, file->ttl, file->sl);
		}
	}

	if(read_size == 0){
	  printf("Client disconnected 2\n");
	  clean_client(socket_args);
	}
	else if(read_size == -1){
			perror("recv failed");
	}

	//Free the socket pointerf
	TAILQ_REMOVE(&active_connections, socket_args, next);
	close(socket_fd);
	free(arg);

	return 0;
}

 
int main(int argc , char *argv[]){
	int client_socket;
	struct sockaddr_in server, client;
	active_connection client_args;

	if (argc < 2){
		perror("Le port à  utiliser est manquant.");
		return EXIT_FAILURE;
	}
	
	int port = atoi(argv[1]);

	TAILQ_INIT(&client_list);
	TAILQ_INIT(&file_list_all);
	TAILQ_INIT(&active_connections);
	
	//Create socket
	if((main_socket = socket(AF_INET , SOCK_STREAM , 0)) == -1){
		perror("socket failed");
	}

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	
	//Handle SIGINT signal
	signal(SIGINT, intHandler);

	// kill "Address already in use" error message
	int yes=1;
	if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt failed");
		exit(1);
	}

	//Bind
	if( bind(main_socket,(struct sockaddr *)&server , sizeof(server)) < 0){
		perror("bind failed");
		return EXIT_FAILURE;
	}
	 
	//Listen
	listen(main_socket , 10);

	printf("Tracker Running\n");

	//Accept new connection
	int socket_size = sizeof(struct sockaddr_in);
	while( (client_socket = accept(main_socket, (struct sockaddr *)&client, (socklen_t*)&socket_size)) ){
		pthread_t client_thread;
		client_args = malloc(sizeof(struct active_connection));
		client_args->sock = client_socket;
		char ipAddr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client.sin_addr.s_addr, ipAddr, INET_ADDRSTRLEN);
		client_args->ip = ipAddr;
		client_args->sys_port = ntohs(client.sin_port);
		// Get remote client system port
		if (getpeername(client_socket, (struct sockaddr *)&client, (socklen_t*) &socket_size) == -1)
		    perror("getpeername");
		TAILQ_INSERT_TAIL(&active_connections, client_args, next);
		printf("Client connected\n");
		// Create new thread to handle connection with client
		if( pthread_create(&client_thread, NULL, threadfunc, (void*) client_args) < 0){
			perror("pthread failed");
			return EXIT_FAILURE;
		}
		pthread_detach(client_thread);
	}
	 
	if (client_socket<0){
		perror("accept failed");
		return EXIT_FAILURE;
	}
	 
	return EXIT_SUCCESS;
}
