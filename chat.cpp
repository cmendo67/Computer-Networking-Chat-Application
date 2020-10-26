
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <typeinfo>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <poll.h>



#define clear() printf("\033[H\033[J")

char** list_of_conected;
struct timeval tv;

struct addrinfo hints, *res, *p;
struct sockaddr_in server_address, client_address;
struct sockaddr *serverpointer = (struct sockaddr *)&server_address;
struct sockaddr *clientpointer = (struct sockaddr *)&client_address;
socklen_t clientlength;

struct sockaddr_storage remoteaddr;
socklen_t addrlen;
fd_set master;
fd_set read_fds;
int fdmax;
int sock;
int newfd;
int yes = 1;
char remoteIP[INET6_ADDRSTRLEN];
int cliensocket[10];
int maxsocket = 10;
int size_of_hints = sizeof(hints);
struct connectinfo {
	int id;
	char *ip;
	char *port;
} connectionlist [10];


int get_sock(char* port){
	//~ struct sockaddr_storage otro_socket;
	
	for(int i = 0; i < 10; i++){
		connectionlist[i].id = i;
		connectionlist[i].ip = "-------------";
		connectionlist[i].port = "----";
	}
	
	//clear hints and res addrinfo structs
	memset(&hints, 0, sizeof(hints));
	memset(&res, 0, sizeof(res));

	
	// bind the sockets
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE || AI_CANONNAME || AI_CANONIDN;
	
	//probably should put a "try" statement here
	getaddrinfo(NULL, port, &hints, &res);
		
	
	//try block for socket creation 
	sock = socket(res->ai_family, res->ai_socktype,res->ai_protocol); 
	if ( sock == -1) {
		fprintf(stderr, "Socket creation encountered a problem. Socket creation failed.\n");
	}
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	
	//23.202.231.169 23.217.138.110 192.168.56.104
	
	
	//try block for binding socket
	int b = bind(sock,res->ai_addr, res->ai_addrlen);
	if( b == -1) {
		close(sock);
		fprintf(stderr, "Binding operation encountered a problem. Bind on port %s failed. Please note that ports 0 - 2023 are reserved for the system and cannot be bound.\n", port);
		return 0;
	} 
	
	//try catch block for listening 
	int listener_socket = listen(sock,10);
	if(listener_socket == -1 ){
		fprintf(stderr, "Listening operation encountered a problem. Socket not listening. \n");
		return 0;
	} else {
		printf("Listening on port %s\n", port);
	}
	
	
	FD_SET(sock, &master);
	fdmax = sock;
	
	return sock;
}


void *get_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

in_port_t get_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);
    return (((struct sockaddr_in6*)sa)->sin6_port);
}





int myip( char ** args, int* count_pntr){
	
	int fileDesc;
	struct ifreq ifr;
	char iface[] = "enp0s8"; 	// can be changed through user input late if need be.
	fileDesc = socket(hints.ai_family,hints.ai_socktype, 0);
	ifr.ifr_addr.sa_family = hints.ai_family;
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
	ioctl(fileDesc, SIOCGIFADDR, &ifr);

	close(fileDesc);
	printf("Your ip is: %s\n",inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );

	return 1;
 }
 
 int help( char ** args, int* count_pntr){
	    printf("help - display all commands available\n");
        printf("myip - display IP address\n");
        printf("connect - connect to another peer\n");
        printf("send - send messages to peers\n");
        printf("myport - display port number you are running on\n");
        printf("list - list all the connections\n");
		printf("exit - Terminate all the connections end exit the program\n");
		return 1;
 }
 
 int myport( char ** args, int* count_pntr){
	 printf("port is %d\n", ntohs(get_port((struct sockaddr *)res->ai_addr)));
	return 1;
}

 
 int connect(char ** args, int* count_pntr ){
	 if(*count_pntr != 3) { 
		 puts("please use comand as follows: connect <valid ip address> <valid port number>. Aborting.");
		 return 1;
	}else{
			struct addrinfo con_hints, *con_res;
			int connect_socket;
			memset(&con_hints, 0, sizeof(con_hints));
			memset(&con_res, 0, sizeof(con_res));
			
			con_hints.ai_family = PF_INET;
			con_hints.ai_socktype = SOCK_STREAM;
			con_hints.ai_flags =  AI_CANONNAME || AI_CANONIDN;
		
		 // below is the connect function using select, which has the capacity to 
		 // manage mulitiple sockets at one time at ease.
		 // what we want to do here is we want to add the socket were connecting to to the 
		 // read_fds or master. we also want to send confirmation of connection
		 // or faliure to the other socket. to do this, we will need both 
		 // connect() and select(). We theoredically dont need a loop because we
		 // will not be constantly listening for texts.
		 
		 //create a new socket from given info i.e. ip and port 
		 printf("ip = %s and port = %s\n", args[1],args[2]);
		 int rv = getaddrinfo(args[1], args[2], &con_hints, &con_res);
		  if (rv != 0)
			{
				perror("getaddrinfo error\n");
				return 1;
			}
		 
		   for (p = con_res; p != NULL; p = p->ai_next) {
				//Initialize socket
				connect_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
				if (connect_socket < 0) continue;
				//Initialize connection
				rv = connect(connect_socket, p->ai_addr, (socklen_t) p->ai_addrlen);
				if (rv == 0) break;
				char* message = "you are now connect to ... another client! ";
				int check = send(connect_socket, message, strlen(message),0);
				if(check < 0) {
					perror("send error");
					return 1;
				}
				close(connect_socket);
				connect_socket = -1;
			}
			
		 freeaddrinfo(con_res);
		 
		 if (connect_socket < 0) //Error creating/connecting socket
			{
				perror("Error creating/connecting socket \n");
				exit(1);
			}

		printf("connect successful\n");

			
		 FD_SET(connect_socket, &master);
		 fdmax = connect_socket;
		 
		 for(int i = 0; i< maxsocket; i++){
			 if (cliensocket[i] == 0){
				cliensocket[i] = fdmax;
				connectionlist[i].ip = args[1];
				connectionlist[i].port = args[2];
				printf("Adding to list of sockets %d at location %d\n" , fdmax, i );
					
			 break;
			 }
		 }
		 
		 printf("%d\n", fdmax);
	return 1;
	}
}
 
 int send(char ** args, int* count_pntr ){
	 int id = atoi(args[0]);
	 char buffer[100];
	 
	 int success= write(sock,args, sizeof(args));
	 if(success < 0){
		 perror("write error");
		 return 1;
		 }
	 
	 return *count_pntr;
 
 }
 
 int terminate(char ** args, int* count_pntr ){

	 
	 int id = atoi(args[0]);
	 FD_CLR(id+4,&master);
	 connectionlist[id].ip = "-------------";
	 connectionlist[id].port = "----";
	 
	 
	 return 1;

 }
 
 int exit(char ** args, int* count_pntr ){
	 FD_ZERO(&master);
	 return 0;
 }
 
 int list(char ** args, int* count_pntr ){
	 
	 printf(" id \t\t\t ip \t\t\t\t\t\t port\n");
	 for(int i = 0; i < 10; i++){
		 printf("%d \t\t\t %s \t\t\t\t\t\t%s\n", connectionlist[i].id, connectionlist[i].ip,connectionlist[i].port);
	 }
	 
	 return 1;
}
 
 
int (*custom_functions[]) (char **, int*) = {
	&myip,
	&connect,
	&help,
	&myport,
	&send,
	&terminate,
	&list,
	&exit
	
};

const char* custom_function_names[] = {
	"myip",
	"connect",
	"help",
	"myport",
	"send",
	"terminate",
	"list",
	"exit"
};


char** parse(char* input, int* j) {
	int bufsize = 1024;
	char** tokens = (char**) malloc(bufsize * sizeof(char*));
	char* token;
	
	
	if (!tokens) {
		exit(EXIT_FAILURE);
	}
	
	// begin tokenizing 
	token = strtok(input, " \n");

	while (token!=NULL) {
		tokens[*j] = token;
		token = strtok( NULL, " \n");
		*j = *j+1;
	}
	
	// I know that the values are being stored in the double pointer because i can 
	// pring them out when i read the double pointer. What seems not to be working
	// is finding the size of the double pointer when doing size of
	// In 64 bit systems both pointer and pointer to pointer are the same
	// size, using size of will not determin the size of the token array.
	// SOLVED: store count ouside of function and pass pointer into function 
	// 		   to keep track of passed values through comand line 
	
	 
	return tokens;
}

char * get_input(){
	char *line = NULL;
	size_t bufsize = 0;
	
	if( getline(&line, &bufsize, stdin) == -1){
		if(feof(stdin)){
			exit(EXIT_SUCCESS);
		} else {
			perror("something went wrong");
			exit(EXIT_FAILURE);
		}
	}
	return line;
}

int check_executable(char** args,int* count_pntr) {
	
	if (args[0] == NULL) {
		return 1;}
	if (strcmp(args[0],"quit") == 0){
		return 0;}
	
	
	for( int i = 0; i < 8; i++){
		if(strcmp(args[0],custom_function_names[i]) == 0){
			return (*custom_functions[i])(args,count_pntr);
		}
	} 
	return 1;
}



int main(int argc, char **argv){
	clear();

	
	if(argv[1] != NULL){ 
		int l = get_sock(argv[1]);
		if (l != -1){
			int l = 1;
			int count_of_cmds = 0;
			int *count_pntr = &count_of_cmds;
			char** parsedInput;
			char* input;
			int status;
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			
			while(l){
			
				count_of_cmds = 0;
				char buf[1024];
				int nbytes;
				read_fds = master;
			
				int selectval = select(fdmax+1, &read_fds, NULL, NULL, &tv);
				printf("select value = %d\n", selectval);
				 if (selectval == 0) {
						printf("user: ");
						input = get_input();
						parsedInput = parse(input, count_pntr);
						status = check_executable(parsedInput,count_pntr);

						free(input);
						free(parsedInput);
				
				
						if(status == 0){
							l = 0;
							puts("Goodbye");
							return 0;
						} 
				 } else {
					printf("fdmax = %d \n",fdmax);
					for(int i =0; i<= fdmax; i++){
						if(int talking_socket = FD_ISSET(sock, &read_fds)){
							printf("talking socket fd = %d\n",talking_socket);
							printf("i value = %d\n", i);
							if(talking_socket != 0) {
								puts("accepting");
								addrlen = sizeof(remoteaddr);
								newfd = accept(sock,(struct sockaddr *)&hints,(socklen_t*)&size_of_hints);
								
								printf("the new socket accepted is %d", newfd);

								if(newfd == -1){
									perror("accept error");
								} else {
									FD_SET(newfd, &master);
									if(newfd > fdmax){
										fdmax = newfd;
									}
									printf("new conncetion from %s on socket %d\n", inet_ntop(remoteaddr.ss_family, get_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN),newfd);
									break;
								}
							} else {
								puts("recieving");
								if((nbytes = recv(i, buf, sizeof buf, 0)) <= 0){
									if(nbytes == 0){
										printf("socket %d disconncected\n", i);
									} else {
										perror("reciving faild");
									}
									close(i);
									FD_CLR(i, &master);
								} else {
									for(int x =0; x <= fdmax; x++){
										if (FD_ISSET(x, &master)) {
											int send_result = send(x, buf, nbytes, 0);
											if(send_result == -1) {
												perror("send");
											}
										}
									}
								}
							}
						}
					}
					
			}
		}
	} 
	} else {
		puts("\nplease provide port number after commnad ; chat_shell <port num goes here>\n");
	return 1;
	}
}









