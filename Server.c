#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include<ctype.h>
#include<unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#include <sys/time.h>
#include <signal.h>

void panic(char *msg);
int client_count = 0;
#define panic(m)	{perror(m); abort();}
#define MAX_CLIENTS 50
#define BUFFER_SIZE 2048

#define time_to_AFK 3 //(12*5sec=1min without interaction)
#define CHECK_time 5 //5seconds
#define name_SIZE 32

// Client structure 
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int u_id;
	char name[name_SIZE];
	int state; //0->AFK, 12->ONLINE (12*5sec=1min without interaction)
} client_t;

//STRUCT ARRAY 
client_t *clients[MAX_CLIENTS];

//Mutex to protec the array of structs 
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


//other fuctions


void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

//----------------------------------------------------------------------------------------
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

//----------------------------------------------------------------------------------------

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

// Add a client to queue

void queue_add_client(client_t *client){
	
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i){
		
		if(!clients[i]){
			clients[i] = client;
			break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

// Remove client from queue

void queue_remove(int u_id){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->u_id == u_id){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

//Send message to everyone

void send_message(char *message, int u_id){
	
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->u_id != u_id){
				if(write(clients[i]->sockfd, message, strlen(message)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

//Communication Handler

void *handle_client(void *arg){
	char buff_out[BUFFER_SIZE];
	char name[name_SIZE];
	int leave_flag = 0;
	

	client_count++;
	client_t *client = (client_t *)arg;

	// Name
	if(recv(client->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("You need to insert a name.\n");
		leave_flag = 1;
		
	} else{
		strcpy(client->name, name);
		client->state = time_to_AFK; //ONLINE
		sprintf(buff_out, "%s has joined\n", client->name);
		printf("%s", buff_out);
		send_message(buff_out, client->u_id);
	}

	bzero(buff_out, BUFFER_SIZE);

	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(client->sockfd, buff_out, BUFFER_SIZE, 0);
		//receive will return 	
		
		if (receive > 0){
		
			client->state = time_to_AFK; //online
			
			if(strlen(buff_out) > 0){
				send_message(buff_out, client->u_id);//goes for everyone
				str_trim_lf(buff_out, strlen(buff_out));
				printf("%s -> %s\n", buff_out, client->name);
			}
		} 
		else if (receive == 0 || strcmp(buff_out, "exit") == 0){
			sprintf(buff_out, "%s has left\n", client->name);
			printf("%s", buff_out);
			send_message(buff_out, client->u_id);
			leave_flag = 1;
		} 
		else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SIZE);
	}

  // Delete client from queue and yield thread

	close(client->sockfd);
  	queue_remove(client->u_id);
  	free(client);//liberta a estrutura
  	client_count--;
  	pthread_detach(pthread_self());

	return NULL;
}


//-----------------------------------------------------------------------------------------------------------	
//-----------------------------------------------------------------------------------------------------------

static void sendPeriodicUpdate(int signo)
{

	if(signo==SIGALRM)
	{
		int i;
		//char client_name[name_SIZE];
		
		for(i=0; i < MAX_CLIENTS; i++)
		{
			if(clients[i]){
				//char message[10];
				if(clients[i]->state == 0){
					//strcpy(message, "AFK\n");
					//send_message(message, clients[i]->u_id);
					printf("AFK -> %s\n", clients[i]->name);
				}
				else{
					clients[i]->state = clients[i]->state - 1;
				}	
			}
		}
	
	}
}

//-----------------------------------------------------------------------------------------------------------	
//-----------------------------------------------------------------------------------------------------------



//here was the thread fuction --------------------------------------

int main(int count, char *args[])
{	struct sockaddr_in cli_addr;
 	 struct sockaddr_in serv_addr;
	 pthread_t tid;

	int listen_sd=0, port;
	int option = 1;
	int connfd = 0;
	int uid = 10;

	if ( count != 2 )
	{
		printf("usage: %s <protocol or portnum>\n", args[0]);
		exit(0);
	}

//--------------------------------------------

	/*---Get server's IP and standard service connection--*/
	if ( !isdigit(args[1][0]) )
	{
		struct servent *srv = getservbyname(args[1], "tcp");
		if ( srv == NULL )
			panic(args[1]);
		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	}
	else
		port = htons(atoi(args[1]));


/*-------------------- create socket -------------------------*/

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if ( listen_sd < 0 )
		panic("socket");

	/*--- bind port/address to socket ---*/

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = INADDR_ANY;    /* any interface */

 /* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listen_sd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
	}
	

	                
	if ( bind(listen_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
		panic("bind");




/* Listen */
  if (listen(listen_sd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");
	
//-----------------------------------------------------------------------------------------------------------	
//-----------------------------------------------------------------------------------------------------------
	
	struct itimerval itv;

	signal(SIGALRM,sendPeriodicUpdate);

	//ualarm(300,300);	
	itv.it_interval.tv_sec = CHECK_time;
	itv.it_interval.tv_usec = 4*10000;
	itv.it_value.tv_sec = CHECK_time;
	itv.it_value.tv_usec = 4*10000;
	setitimer (ITIMER_REAL, &itv, NULL);	
	
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------


	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listen_sd, (struct sockaddr*)&cli_addr, &clilen); 


		/* Check if max clients is reached */
		if((client_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd); 
			continue;
		}

		/* Client settings */ 
		client_t *client = (client_t *)malloc(sizeof(client));
		client->address = cli_addr;
		client->sockfd = connfd;
		client->u_id = uid++;

		/* Add client to the queue and fork thread */
		queue_add_client(client);
		pthread_create(&tid, NULL, &handle_client, client); 
	}
	

	return EXIT_SUCCESS;
}
