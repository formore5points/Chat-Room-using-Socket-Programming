#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

int clientCount = 0;
int roomCount = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct client{

	int index;
	int sockID;
	struct sockaddr_in clientAddr;
	int len;
    char nick[50];
    int current_room;

};

struct room{
	int id;
	int is_private;
	char room_name[50];
	char password[10];
}room;

struct client Client[1024];
pthread_t thread[1024];
struct room Rooms[10];


void * doNetworking(void * ClientDetail){

	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
    int current_room=clientDetail->current_room;
	char *lobby="You are in lobby now.\n";
    send(clientSocket,lobby,strlen(lobby),0);
	sleep(1);
    char data[1024];
    char *nick="Please choose a nick name : ";  
    send(clientSocket,nick,strlen(nick),0);
    int read = recv(clientSocket,data,1024,0);
	data[read] = '\0';
	printf("Client name %s number %d connected.\n",data,index + 1);
	strcpy(Client[index].nick,data);
    char *nick_name=data;
    strcat(nick_name, ": ");
	

	while(1){

		char data[1024];
		int read = recv(clientSocket,data,1024,0);
		data[read] = '\0';
		char output[1024];
        
		if(strcmp(data,"-list") == 0){
			int l=0;
			l += snprintf(output + l,1024,"Lobby\n");
			for(int j=0;j<clientCount;j++){
				if(Client[j].current_room==0)
					l += snprintf(output + l,1024,"%s\n",Client[j].nick);
			}
			for(int i=0;i<roomCount;i++){
				if(Rooms[i].id!=-1){
					if(Rooms[i].is_private!=1){
						l += snprintf(output + l,1024,"Room %s\n",Rooms[i].room_name);
							for(int j=0;j<clientCount;j++){
								if(Client[j].current_room==Rooms[i].id)
									l += snprintf(output + l,1024,"%s\n",Client[j].nick);
							}
					}else{
						l += snprintf(output + l,1024,"Room %s is private.\n",Rooms[i].room_name);
					}
				}
			}	
			send(clientSocket,output,1024,0);

		}

		if(strcmp(data,"-msg") == 0){
			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';
            char message[1024];
            strcpy(message, nick_name);
            strcat(message, data);
            int i;
            for(i=0;i<clientCount;i++){
                if(Client[index].current_room==Client[i].current_room){
                    send(Client[i].sockID,message,1024,0);
                }
            }
		}

		if(strcmp(data,"-whoami") == 0){
			char helper[1024]="Your nick name is : ";
			strcat(helper,nick_name);
			send(clientSocket,helper,strlen(helper),0);
		}

		if(strcmp(data,"-exit") == 0){
			int i;
			for(i=0;i<clientCount;i++){
                    send(Client[i].sockID,data,1024,0);              
            }
			exit(0);
		}

		if(strcmp(data,"-create") == 0 ){
			if(Client[index].current_room==0){
			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';
			char *room_name=data;	
			Rooms[roomCount].id=roomCount+1;
			Rooms[roomCount].is_private=0;
			strcpy(Rooms[roomCount].password,"");
			strcpy(Rooms[roomCount].room_name,room_name);
			Client[index].current_room=roomCount+1;
			roomCount++;
			char message[1024]="Room succesfully created.\nYou are in room now.\n";
			send(clientSocket,message,strlen(message),0);
			}
			else{
				char *errmessage="You can not create room in a room!\n";
				send(clientSocket,errmessage,strlen(errmessage),0);
			}
			
		}
		if(strcmp(data,"-pcreate") == 0 ){
			if(Client[index].current_room==0){
			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';					
			strcpy(Rooms[roomCount].room_name,data);
			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';
			char *password=data;
			Rooms[roomCount].id=roomCount+1;
			Rooms[roomCount].is_private=1;
			strcpy(Rooms[roomCount].password,password);								
			Client[index].current_room=roomCount+1;
			roomCount++;			
			char message[1024]="Private room succesfully created.\nYou are in room now.\n";
			send(clientSocket,message,strlen(message),0);
			}
			else{
				char *errmessage="You can not create room in a room!\n";
				send(clientSocket,errmessage,strlen(errmessage),0);
			}
			
		}
		if(strcmp(data,"-quit") == 0 ){
			if(Client[index].current_room!=0){
				int old_room=Client[index].current_room;
				int control=0;
				Client[index].current_room=0;
				char message[1024]="Quited room succesfully.\nYou are in lobby now.\n";
				send(clientSocket,message,strlen(message),0);
				for(int i=0;i<clientCount;i++){
					if(Client[i].current_room==old_room){
						control=1;
					}
				}
				if(control==0){
					char *roommessage="Room closed.\n";
					send(clientSocket,roommessage,strlen(roommessage),0);
					Rooms[old_room].id=-1;
					Rooms[old_room].is_private=0;
					strcpy(Rooms[roomCount].password,"");
					strcpy(Rooms[roomCount].room_name,"");
				}
			}
			else{
				char *errmessage="You are not in a room already.!\n";
				send(clientSocket,errmessage,strlen(errmessage),0);
			}
			
		}
		if(strcmp(data,"-enter") == 0 ){
			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';
			char *room_name=data;
			int control=0;
			int room_index=-1;
			for(int i=0;i<roomCount;i++){
				if(strcmp(room_name,Rooms[i].room_name)==0){
					control=1;
					room_index=i;
				}
			}

		if(control==1){
			if(Rooms[room_index].is_private==0){
				Client[index].current_room=Rooms[room_index].id;
				char message[1024]="You entered the room successfully.\n";
				send(clientSocket,message,strlen(message),0);
			}else{
				char *ask_pass="-pass";
				send(clientSocket,ask_pass,strlen(ask_pass),0);
				read = recv(clientSocket,data,1024,0);
				data[read] = '\0';
				char *pass=data;
				if(strcmp(Rooms[room_index].password,pass)==0){
					Client[index].current_room=Rooms[room_index].id;
					char message[1024]="You entered the private room successfully.\n";
					send(clientSocket,message,strlen(message),0);
				}else{
					char *errmessage="Wrong Password!\n";
					send(clientSocket,errmessage,strlen(errmessage),0);
				}
			}
		}else{
				char *errmessage="Room not found!\n";
				send(clientSocket,errmessage,strlen(errmessage),0);
		}


		}
	}
	return NULL;
}

int main(){

	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(3205);
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);


	if(bind(serverSocket,(struct sockaddr *) &serverAddr , sizeof(serverAddr)) == -1) return 0;

	if(listen(serverSocket,1024) == -1) return 0;

	printf("Server started.\n");

	while(1){

		Client[clientCount].sockID = accept(serverSocket, (struct sockaddr*) &Client[clientCount].clientAddr, &Client[clientCount].len);
		Client[clientCount].index = clientCount;

		pthread_create(&thread[clientCount], NULL, doNetworking, (void *) &Client[clientCount]);

		clientCount ++;
 
	}

	for(int i = 0 ; i < clientCount ; i ++)
		pthread_join(thread[i],NULL);

}