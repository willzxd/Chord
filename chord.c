#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <math.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "Sha1.h"

#define KEY_SIZE ((long)pow(2, 32))
#define FINGERTABLE_SIZE ((int)log2(KEY_SIZE))

typedef struct {
	long id;
	char ip[100];
	int port;
} node;

typedef struct {
	long start;
	long end;
	long id;
} finger;

node *predecessor0;
node *predecessor1;
node *successor0;
node *successor1;
node *current;
finger *fingerTable;
int *quitUpdate;
int *idtmp;

struct sockaddr_in client_addr;

/* =================Functions============== */
/**
Creat a new ring
*/
void createRing(char*argv);
/**
main setupServer here
*/
int setupServer(char*argv);
/**
handle connection
*/
void handle(int sock);
/**
join an old ring
*/
void join(char**argv);
/**
get node id
*/
long getId(char *str);
/**
print node info
*/
void printNode(node nodeToPrint);
/**
request other nodes to find the position
*/
void sendNtoN(node nodeInfo[], int numberOfNode, char* message, node requestNode);
/**
*/
void sendStoN(char *msg, char strs[][1000], int size, node requestNode);
/**
update finger table
*/
void updateFTable(long id1, long id2, int a);
/**
update finger table
*/
void insertFTable(long start, long nodeId);
/**
*/
int joinIn(node nodeNew, node *preToJoin, node *sucToJoin);
/**
uersinterface for users
*/
void ui(void);
/**
init finger table
*/
void initFTable(void);
/**
like stol in c++
*/
long stringToLong();
/**
readaline everytime 
*/
int readaline(int socket, char *bufferpt, size_t length);
/**
*/
node lookup(long key, node node_source);
/**
*/
void quit();

static void wait_for_children(int sig){
	while (waitpid(-1, NULL, WNOHANG) > 0){
	};
}
//void deleteNode(node node_dead);

/*================MAIN==================*/
int main(int argc, char** argv){
	if(argc != 2 && argc != 4){
		printf("usage: %s [Port]\n", argv[1]);
		printf("usage: %s [my port] [server ipaddress] [server port]\n", argv[1]);
		exit(1);
	}

	//init current node, 2 predecessors, 2 successor0cessors
	current = mmap(NULL, sizeof(current), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	current->id = 0;
	current->port = 0;
	strcpy(current->ip, "\0");

	predecessor0 = mmap(NULL, sizeof(predecessor0), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	predecessor0->id = 0;
	predecessor0->port = 0;
	strcpy(predecessor0->ip, "\0");

	predecessor1 = mmap(NULL, sizeof(predecessor1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	predecessor1->id = 0;
	predecessor1->port = 0;
	strcpy(predecessor1->ip, "\0");

	successor0 = mmap(NULL, sizeof(successor0), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	successor0->id = 0;
	successor0->port = 0;
	strcpy(successor0->ip, "\0");

	successor1 = mmap(NULL, sizeof(successor1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	successor1->id = 0;
	successor1->port = 0;
	strcpy(successor1->ip, "\0");

	fingerTable = mmap(NULL, sizeof(FINGERTABLE_SIZE * sizeof(finger)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	quitUpdate = mmap(NULL, sizeof(quitUpdate),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*quitUpdate = 0;

	//
	CurrentInfo(argv[1]);

	if(argc == 2){
		printf("Only two arguments.Create a new ring.\n");
		createRing(argv[1]);
	} 
	else{
		printf("Only four arguments. Join a old ring.\n");
		join(argv);
	}

	int pid0 = fork();

	if (pid0 == 0){
		//printf("Need a function send "IS_ALIVE" here");
	} 
	else{
		if (pid0 == -1){
			perror("fork");
			return 1;
		} 
		else{
			
			int pid = fork();
			if (pid == 0){
				//printf("===MAIN: I will fork and set up server.===\n");
				setupServer(argv[1]);
				return 0;
			} 
			else{
				if (pid == -1){
					perror("fork");
					return 1;
				} 
				else{
					//printf("I will deal with ui.\n");
					ui();
					return 0;
				}
			}

			return 0;
		}
	}

}

int CurrentInfo(char* port){

/*
Get ip address. 
Code from: http://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux
*/
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		//in OSX change "eth0" to "en0"
        if((strcmp(ifa->ifa_name,"eth0")==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host); 
        }
    }

    freeifaddrs(ifaddr);

	
	current->port = atoi(port);
	strncpy(current->ip, host, strlen(host));
	//connect ip and port
	char ipPort[30];
	strcpy (ipPort, current->ip);
    strcat (ipPort,": ");
    strcat (ipPort, port);
    printf("MyAdress is %s.\n", ipPort);
    current->id = getId(ipPort);
    //printf("Your position is %ld\n", (&current)->id);
	//printf("In CurrentInfo\n");
	//printf("Current ip: %s Current port: %s Current id: %ld\n", (&current)->ip, (&current)->port, (&current)->id);
}

/**
Get the Node ID
*/
long getId(char *str){

	sha1nfo i;

	char ipPort[30], tmp[30];
	if (strcmp(str, "\0") == 0){
		//connect ip and port
		strcpy (ipPort, current->ip);
    	strcat (ipPort,": ");
    	sprintf(tmp, "%d", current->port);
    	strcat (ipPort, tmp);
	} else
		strcpy(ipPort, str);

	sha1_init(&i);
	sha1_write(&i, ipPort, strlen(ipPort));


	uint8_t* hash = sha1_result(&i);
	uint32_t b0 = hash[0] | hash[1] << 8 | hash[2] << 16 | hash[3] << 24;
	uint32_t b1 = hash[4] | hash[5] << 8 | hash[6] << 16 | hash[7] << 24;
	uint32_t b2 = hash[8] | hash[9] << 8 | hash[10] << 16 | hash[11] << 24;
	uint32_t b3 = hash[12] | hash[13] << 8 | hash[14] << 16 | hash[15] << 24;
	uint32_t b4 = hash[16] | hash[17] << 8 | hash[18] << 16 | hash[19] << 24;
	
	long id;
	id = b0 ^ b1 ^ b2 ^ b3 ^ b4;
	return id;
}

/*
In main function: Creat a new chord ring
*/
void createRing(char* argv){
	idtmp = mmap(NULL, sizeof(idtmp), PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	(*idtmp) = 0;

	predecessor0->id = current->id;
	strcpy(predecessor0->ip, current->ip);
	predecessor0->port = current->port;

	predecessor1->id = current->id;
	strcpy(predecessor1->ip, current->ip);
	predecessor1->port = current->port;

	successor0->id = current->id;
	strcpy(successor0->ip, current->ip);
	successor0->port = current->port;

	successor1->id = current->id;
	strcpy(successor1->ip, current->ip);
	successor1->port = current->port;

	initFTable();
}

/**Init figner talbe
1. When create a new chord ring
2. When a node joins in a old chord ring
3.
*/
void initFTable(){
	printf("initFTable: Init figner table begin...\n");
	int i;
	long temp, bound;
	for (i = 0; i <= FINGERTABLE_SIZE - 1; i++){

		temp = current->id + pow(2, i);
		if (temp > KEY_SIZE - 1){
			bound = temp - KEY_SIZE;
		}
		else{
			bound = temp;
		}
		fingerTable[i].start = bound;

		temp = current->id + pow(2, i + 1);
		if (temp > KEY_SIZE - 1){
			bound = temp - KEY_SIZE;
		}
		else{
			bound = temp;
		}
		fingerTable[i].end = bound;

		fingerTable[i].id = current->id;
	}
	fingerTable[FINGERTABLE_SIZE - 1].end = current->id;
	printf("iniFTable: Finish init finger talbe.\n");
}

int readaline(int socket, char *bufferpt, size_t length){
	char *bufx = bufferpt;
	static char *bytepointer;
	static int number_byte = 0;
	static char buf[1000];
	char c;

	while (--length > 0){
		if (--number_byte <= 0){
			number_byte = recv(socket, buf, sizeof(buf), 0);
			if (number_byte < 0){
				if (errno == EINTR){
					length++;
					continue;
				}
				return -1;
			}
			if (number_byte == 0)
				return 0;
			bytepointer = buf;
		}
		c = *bytepointer++;
		*bufferpt++ = c;
		if (c == '\n'){
			*bufferpt = '\0';
			if (*(bufferpt - 1) == '\n')
				*(bufferpt - 1) = '\0';
			return bufferpt - bufx;
		}
	}
	return -1;
}

/*
Convert String to Long
*/
long stringToLong(char *str){//Can use stol in c++ instead.

	int length = strlen(str);
	int i, dig, power;
	long result = 0;
	for (i = 0; i <= length - 1; i++){
		dig = str[i] - 48;
		power = length - i - 1;
		result += dig * pow(10, power);
		//printf("%d, %d, %ld\n", dig, power, result);

	}

	//printf("result=%ld\n", result);
	return result;
}

/*
In main function: Join in a ring
*/
void join(char** argv){
	// struct sockaddr_in {
	// 	short int sin_family; /* 通信类型 */
	// 	unsigned short int sin_port; /* 端口 */
	// 	struct in_addr sin_addr; /* Internet 地址 */
	// 	unsigned char sin_zero[8]; /* 与sockaddr结构的长度相同*/
	// };
	struct sockaddr_in serveraddr, clientaddr;
	int serversock;
	int n;
	char sendline[1000];
	char recvline[1000];

	//init parameters
	serversock = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	
	if (inet_addr(argv[2]) != -1){
		printf("Join: server ip is: %s\n", argv[2]);
		serveraddr.sin_addr.s_addr = inet_addr(argv[2]);
	}
	else{
		printf("Join error: server address error");
	}

	printf("Join: server port is: %s\n", argv[3]);
	serveraddr.sin_port = htons(atoi(argv[3]));

	//connect to server
	printf("Join: connect to the server...\n");
	n = connect(serversock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if (n == 0){
		printf("Join: connceted!\n");
	}
	else{
		printf("error:%s\n",strerror(errno));
		perror("Join: connect failed.\n");
	}
	

	strcpy(sendline, "JOIN");
	strcat(sendline, "\n");
	sendto(serversock, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Join: send join message:%s\n", sendline);

	sprintf(sendline, "%ld", current->id);
	strcat(sendline, "\n");
	sendto(serversock, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Join: send my id:%s\n", sendline);

	strcpy(sendline, current->ip);
	strcat(sendline, "\n");
	sendto(serversock, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Join: send my ip:%s\n", sendline);

	sprintf(sendline, "%d", current->port);
	strcat(sendline, "\n");
	sendto(serversock, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Join: send my port:%s\n", sendline);


	n = readaline(serversock, recvline, 1000);
	//printf("recv:%s\n", recvline);

	if (strcmp(recvline, "JOIN_SUCC") == 0){
		printf("JOIN_SUCC: begin to read receive message\n");
		n = readaline(serversock, recvline, 1000);
		predecessor0->id = stringToLong(recvline);
		printf("Join: receive predecessor id:%s\n", recvline);

		n = readaline(serversock, recvline, 1000);
		strcpy(predecessor0->ip, recvline);
		printf("Join: receive predecessor ip:%s\n", recvline);

		n = readaline(serversock, recvline, 1000);
		predecessor0->port = atoi(recvline);
		printf("Join: receive predecessor port:%s\n", recvline);

		n = readaline(serversock, recvline, 1000);
		successor0->id = stringToLong(recvline);
		printf("Join: receive successor id:%s\n", recvline);

		n = readaline(serversock, recvline, 1000);
		strcpy(successor0->ip, recvline);
		printf("Join: receive successor ip:%s\n", recvline);

		n = readaline(serversock, recvline, 1000);
		successor0->port = atoi(recvline);
		printf("Join: receive successor port:%s\n", recvline);

		initFTable();
		printf("Join: init fingertable finished.\n");
		printNode(* current);
		printNode(* predecessor0);
		printNode(* successor0);
		
		node newN[2];
		newN[0] = *current;
		newN[1].id = 0;
		printf("Send UPDATE info to successor.\n");
		sendNtoN(newN, 2, "UPDATE", *successor0);

	}
	close(serversock);

}
/*
In main function
*/
int setupServer(char* argv){
	// printf("Im setupServer.\n");
	// return 0;
	int listensock;
	//http://www.haodaima.net/art/1719250
	struct sigaction sigact;
	struct addrinfo tempadd, *resadd;
	int ruaddr = 1;

	memset(&tempadd, 0, sizeof tempadd);//init zero
	tempadd.ai_family = AF_INET;
	tempadd.ai_socktype = SOCK_STREAM;
	char ip[30];
	strcpy (ip, current->ip);
	if (getaddrinfo(ip, argv, &tempadd, &resadd) != 0){
		perror("getaddrinfo error");
		return 1;
	}

	listensock = socket(resadd->ai_family, resadd->ai_socktype,
			resadd->ai_protocol);
	//change option of listensock
	printf("setupServer: configure opt.\n");
	if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &ruaddr, sizeof(int))
			== -1){
		perror("setsockopt error");
		return 1;
	}
	if (bind(listensock, resadd->ai_addr, resadd->ai_addrlen) == -1){
		perror("bind error");
		return 1;
	}
	//begin listen
	if (listen(listensock, 20) == -1){
		perror("listen error");
		return 1;
	}
	printf("setupServer: listen begin...\n");
	freeaddrinfo(resadd);

	// sigact.sa_handler = wait_for_children;
	// sigemptyset(&sigact.sa_mask);
	// sigact.sa_flags = SA_RESTART;
	// if (sigaction(SIGCHLD, &sigact, NULL ) == -1){
	// 	perror("sigaction");
	// 	return 1;
	// }

	while (1){
		printf("setupServer: accept begin...\n");
		socklen_t addsize = sizeof(struct sockaddr_in);

		
		int clientsock = accept(listensock, (struct sockaddr*) &client_addr, &addsize);
		if (clientsock == -1){
			perror("accept error");
			return 0;
		}
		printf("setupServer: get clientsock:%d\n", clientsock);
		printf("Established a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
		
		int pid;
		pid = fork();
		if (pid == 0){
			close(listensock);
			handle(clientsock);
			exit(0);
		}
		else{
			if (pid == -1){
				perror("fork");
				return 1;
			} 
			else{
				close(clientsock);
			}
		}
	}

	close(listensock);

	return 0;
}
/*
After a connection, handle the messages.
*/
void handle(int sock){
	//printf("Im handle.\n");
	//return 0;

	int n;
	char msgR[1000];
	char msgS[1000];

	n = readaline(sock, msgR, 1000);
	

	if (strcmp(msgR, "JOIN") == 0){
		printf("====Handle: Got JOIN====\n");

		node node_new;
		n = readaline(sock, msgR, 1000);
		printf("Got JOIN:%s\n", msgR);
		(&node_new)->id = stringToLong(msgR);

		n = readaline(sock, msgR, 1000);
		printf("Got JOIN:%s\n", msgR);
		strcpy((&node_new)->ip, msgR);

		n = readaline(sock, msgR, 1000);
		printf("Got JOIN:%s\n", msgR);
		(&node_new)->port = atoi(msgR);

		node *pre_toJoin = malloc(sizeof(node));
		node *suc_toJoin = malloc(sizeof(node));
		int result = joinIn(node_new, pre_toJoin, suc_toJoin);

		if (result == 1 || result == 2){
			sendto(sock, "JOIN_SUCC\n", 10, 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("=====Handle: JOIN_SUCC=====\n");

			sprintf(msgS, "%ld", pre_toJoin->id);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.predecessor's id:%s\n", msgS);
			strcpy(msgS, pre_toJoin->ip);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.predecessor's ip:%s\n", msgS);
			sprintf(msgS, "%d", pre_toJoin->port);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.predecessor's port:%s\n", msgS);

			sprintf(msgS, "%ld", suc_toJoin->id);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.successor's id:%s\n", msgS);
			strcpy(msgS, suc_toJoin->ip);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.successor's ip:%s\n", msgS);
			sprintf(msgS, "%d", suc_toJoin->port);
			strcat(msgS, "\n");
			sendto(sock, msgS, strlen(msgS), 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			printf("handle.JOIN_SUCC.successor's port:%s\n", msgS);

			if (result == 2){
				node nodes_info[1];
				nodes_info[0] = node_new;
				int number_node = 1;
				sendNtoN(nodes_info, number_node, "PRE",
						*suc_toJoin);
			}
		} 
		else{
			sendto(sock, "JOIN_WAIT\n", 10, 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));
			node nodes_info[1];
			nodes_info[0] = node_new;
			int number_node = 1;
			sendNtoN(nodes_info, number_node, "JOIN_FD", *successor0);
		}
	} 
	else if (strcmp(msgR, "JOIN_FD") == 0){
		printf("=====Handle: Got JOIN_FD=====\n");
		node node_new;
		n = readaline(sock, msgR, 1000);
		(&node_new)->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy((&node_new)->ip, msgR);
		n = readaline(sock, msgR, 1000);
		(&node_new)->port = atoi(msgR);
		node *pre_toJoin = malloc(sizeof(node));
		node *suc_toJoin = malloc(sizeof(node));
		int result = joinIn(node_new, pre_toJoin, suc_toJoin);

		if (result == 1 || result == 2){
			node nodes_info[2];
			nodes_info[0] = *pre_toJoin;
			nodes_info[1] = *suc_toJoin;
			int number_node = 2;
			sendNtoN(nodes_info, number_node, "JOIN_SUCC", node_new);

			if (result == 2){
				node nodes_info[1];
				nodes_info[0] = node_new;
				int number_node = 1;
				sendNtoN(nodes_info, number_node, "PRE",
						*suc_toJoin);
			}
		} 
		else{
			sendto(sock, "JOIN_WAIT\n", 10, 0,
					(struct sockaddr *) &client_addr, sizeof(client_addr));

			node nodes_info[1];
			nodes_info[0] = node_new;
			int number_node = 1;
			sendNtoN(nodes_info, number_node, "JOIN_FD", *successor0);
		}
	} 
	else if (strcmp(msgR, "JOIN_SUCC") == 0){
		printf("====Handle: Got %s=====\n", msgR);

		n = readaline(sock, msgR, 1000);
		predecessor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(predecessor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		predecessor0->port = atoi(msgR);
		n = readaline(sock, msgR, 1000);
		successor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(successor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		successor0->port = atoi(msgR);
		initFTable();

		/*
		 printf("predecessor0:\n");
		 printNode(*predecessor0);
		 printf("successor0:\n");
		 printNode(*successor0);
		 */
		//updateFTable();
		node node_new[2];
		node_new[0] = *current;
		node_new[1].id = 0;
		sendNtoN(node_new, 2, "UPDATE", *successor0);

	} 
	else if (strcmp(msgR, "PRE") == 0){
		printf("====Handle: Got %s=====\n", msgR);

		n = readaline(sock, msgR, 1000);
		predecessor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(predecessor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		predecessor0->port = atoi(msgR);
		/*
		 printf("predecessor0:\n");
		 printNode(*predecessor0);
		 printf("successor0:\n");
		 printNode(*successor0);
		 */
	} 
	else if (strcmp(msgR, "SUCC") == 0){
		printf("====Handle: Got %s=====\n", msgR);

		n = readaline(sock, msgR, 1000);
		successor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(successor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		successor0->port = atoi(msgR);
		/*
		 printf("predecessor0:\n");
		 printNode(*predecessor0);
		 printf("successor0:\n");
		 printNode(*successor0);
		 */
	} 
	else if (strcmp(msgR, "UPDATE") == 0){
		printf("====Handle: Got %s=====\n", msgR);

		node node_new;
		n = readaline(sock, msgR, 1000);
		node_new.id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(node_new.ip, msgR);
		n = readaline(sock, msgR, 1000);
		node_new.port = atoi(msgR);
		

		int power;
		n = readaline(sock, msgR, 1000);
		power = atoi(msgR);
		n = readaline(sock, msgR, 1000);
		n = readaline(sock, msgR, 1000);
		if (node_new.id != current->id){
			updateFTable(node_new.id, -1, 1);
			if (power < FINGERTABLE_SIZE ){
				int i;
				int j = 0;
				char results[FINGERTABLE_SIZE * 2][1000];
				for (i = power; i <= FINGERTABLE_SIZE - 1; i++){

					long bound = 0;
					if (node_new.id + (long) pow(2, i) >= KEY_SIZE ){
						bound = node_new.id + (long) pow(2, i) - KEY_SIZE;
					} 
					else{
						bound = node_new.id + (long) pow(2, i);
					}

					if (current->id >= bound){
						sprintf(results[j++], "%ld", bound);
						sprintf(results[j++], "%ld", current->id);
					} 
					else if(successor0->id > current->id && current->id < predecessor0->id
							&& predecessor0->id >= successor0->id && bound > current->id){
						sprintf(results[j++], "%ld", bound);
						sprintf(results[j++], "%ld", current->id);
					} 
					else{
						break;
					}
				}
				power = i;
				if (j > 0)
					sendStoN("FTABLE_UPDATE", results, j, node_new);
			}
			node nodes_new[2];
			nodes_new[0] = node_new;
			nodes_new[1].id = power;
			sendNtoN(nodes_new, 2, "UPDATE", *successor0);
		}
	} 
	else if (strcmp(msgR, "FTABLE_UPDATE") == 0){
		printf("====Handle: Got %s=====\n", msgR);
		int size;
		n = readaline(sock, msgR, 1000);
		size = atoi(msgR);
		
		int i;
		long start;
		long nodeId;
		for (i = 0; i <= size - 1; i += 2){

			n = readaline(sock, msgR, 1000);
			start = stringToLong(msgR);
			n = readaline(sock, msgR, 1000);
			nodeId = stringToLong(msgR);
			insertFTable(start, nodeId);
		}
	} 
	else if (strcmp(msgR, "LEAVE_PRE") == 0){
		printf("====Handle: Got %s=====\n", msgR);

		n = readaline(sock, msgR, 1000);
		predecessor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(predecessor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		predecessor0->port = stringToLong(msgR);
		/*
		 printf("predecessor0:\n");
		 printNode(*predecessor0);
		 printf("successor0:\n");
		 printNode(*successor0);
		 */
	} 
	else if (strcmp(msgR, "LEAVE_SUC") == 0){
		printf("====Handle: Got %s=====\n", msgR);
		n = readaline(sock, msgR, 1000);
		successor0->id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(successor0->ip, msgR);
		n = readaline(sock, msgR, 1000);
		successor0->port = atoi(msgR);
		/*
		 printf("predecessor0:\n");
		 printNode(*predecessor0);
		 printf("successor0:\n");
		 printNode(*successor0);
		 */
	} 
	else if (strcmp(msgR, "UPDATE_LEAVE") == 0){
		printf("====Handle: Got %s=====\n", msgR);
		node node_leave;
		n = readaline(sock, msgR, 1000);
		node_leave.id = stringToLong(msgR);		
		n = readaline(sock, msgR, 1000);
		strcpy(node_leave.ip, msgR);		
		n = readaline(sock, msgR, 1000);
		node_leave.port = atoi(msgR);		
		node node_suc;
		n = readaline(sock, msgR, 1000);
		node_suc.id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(node_suc.ip, msgR);	
		n = readaline(sock, msgR, 1000);
		node_suc.port = atoi(msgR);	
		if (node_leave.id != current->id){
			updateFTable(node_leave.id, node_suc.id, 0);
			node nodes_new[2];
			nodes_new[0] = node_leave;
			nodes_new[1] = node_suc;
			sendNtoN(nodes_new, 2, "UPDATE_LEAVE", *successor0);
		} else{
			*quitUpdate = 1;
		}
	} 
	//Search part
	else if (strcmp(msgR, "LOOKUP") == 0){
		printf("=====Handle: Get LOOKUP Message=====\n");

		long key;
		node node_source;
		n = readaline(sock, msgR, 1000);
		printf("handle: lookup message:%s\n", msgR);
		key = stringToLong(msgR);

		n = readaline(sock, msgR, 1000);
		printf("handle: lookup message:%s\n", msgR);
		n = readaline(sock, msgR, 1000);
		printf("handle: lookup message:%s\n", msgR);
		n = readaline(sock, msgR, 1000);
		node_source.id = stringToLong(msgR);
		printf("handle: lookup message:%s\n", msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(node_source.ip, msgR);
		printf("handle: lookup message:%s\n", msgR);
		n = readaline(sock, msgR, 1000);
		node_source.port = atoi(msgR);
		printf("handle: lookup message:%s\n", msgR);
		if (node_source.id == current->id){
			printf("Key not found\n");
		} 
		else{
			node result;
			result = lookup(key, node_source);
			if (result.id != -1){
				printf("The key belongs to the node:\n");
				printNode(*current);
				printf("Not found.\n");
				node node_found[1];
				node_found[0] = *current;
				sendNtoN(node_found, 1, "FOUND", node_source);
			}
		}
	}
	//Receive a "FOUND" message, print the message out  
	else if (strcmp(msgR, "FOUND") == 0){
		printf("handle: Got FOUND: %s\n", msgR);
		node node_found;
		n = readaline(sock, msgR, 1000);
		node_found.id = stringToLong(msgR);
		n = readaline(sock, msgR, 1000);
		strcpy(node_found.ip, msgR);
		n = readaline(sock, msgR, 1000);
		node_found.port = atoi(msgR);
		printf("Key found on node:\n");
		printNode(node_found);
	} 
	//else if (strcmp(msgR, "IS_ALIVE") == 0){
		//printf("====Handle: Got %s=====\n", msgR);
		//send back a confirm message
	//}
}

/*
In main function: An interface to give instructions
*/
void ui(){
	char cmd[10000];
	while (1){
		printf("===========================CMD==========================\ninfo\tcurrent node info\nrange\tshow responsible key range\nfinger\tfinger table\n");
		printf("Please enter your search key (or type quit to leave):\n");
		printf("========================================================\n");
		printf("Current node: ");
		printNode(*current);
		printf("\n");
		printf("Predecessor: ");
		printNode(*predecessor0);
		printf("\n");
		printf("Successor: ");
		printNode(*successor0);
		printf("\n");
		printf("========================================================\n");
		fgets(cmd, sizeof(cmd), stdin);
		if (strcmp(cmd, "info\n") == 0){
			printf("Current node: ");
			printNode(*current);
			printf("\n");
			printf("Predecessor: ");
			printNode(*predecessor0);
			printf("\n");
			printf("Successor: ");
			printNode(*successor0);
			printf("\n");
		} 
		else if (strcmp(cmd, "ftable\n") == 0){//print fingertable
			int i;
			for (i = 0; i <= FINGERTABLE_SIZE - 1; i++){
				printf("[%ld, %ld) -> %ld\n", fingerTable[i].start, fingerTable[i].end, fingerTable[i].id);
			}
		} 
		else if (strcmp(cmd, "quit\n") == 0){
			quit();
			kill(0, SIGTERM);
			exit(0);
		} 
		else if (strcmp(cmd, "range\n") == 0){
			printf("From %ld to %ld\n", predecessor0->id, current->id);
		}
		else if (strcmp(cmd, "\n") != 0){
			long key;
			key = getId(cmd);
			printf("Hash value is %ld\n", key);
			node node_result;
			node_result = lookup(key, *current);
			if (node_result.id != -1){
				printf("Key found on node:\n");
				printNode(node_result);
			}
		}
	}
	return;
}

/*
print a node info
*/
void printNode(node i){
	printf("Current ip: %s Current port: %d Current id: %ld\n", (&i)->ip, (&i)->port, (&i)->id);
}

/*
From interface cmd: quit
*/
void quit(){
	printf("=====Quiting begins.=====\n");
	if (predecessor0->id == current->id && successor0->id == current->id){
		printf("Last node quits.\n");
		return;
	}
	node nodesInfo[2];
	int number_node = 2;
	nodesInfo[0] = *current;
	nodesInfo[1] = *successor0;
	sendNtoN(nodesInfo, number_node, "UPDATE_LEAVE", *successor0);

	while (*quitUpdate == 0){
		sleep(1);
	}

	number_node = 1;

	nodesInfo[0] = *predecessor0;
	sendNtoN(nodesInfo, number_node, "LEAVE_PRE", *successor0);

	nodesInfo[0] = *successor0;
	sendNtoN(nodesInfo, number_node, "LEAVE_SUC", *predecessor0);
}

/*
From interface cmd: search a key
*/
node lookup(long key, node i){
	int j;
	//find whether the key is in the range  of current node
	if (current->id > predecessor0->id){
		if (key <= current->id && key > predecessor0->id)
			j = 1;
		else
			j = 0;
	} 
	else if (current->id < predecessor0->id){
		if (key <= current->id || key > predecessor0->id)
			j = 1;
		else
			j = 0;
	}
	else if (current->id == predecessor0->id){
		j = 1;
	}

	//if in the range of current node, return current node
	if (j != 0)
		return *current;
	else{
		//send the request to other node to find out the result
		node nodeInfo[2];
		nodeInfo[0].id = key;
		nodeInfo[1] = i;

		int k;
		for (k = FINGERTABLE_SIZE - 1; k >= 0; k--){
			if (fingerTable[k].end > fingerTable[k].start){
				if (key >= fingerTable[k].start && key < fingerTable[k].end){
					sendNtoN(nodeInfo, 2, "LOOKUP", *successor0);
					break;
				}
			} 
			else if (fingerTable[k].end < fingerTable[k].start){
				if (key >= fingerTable[k].start || key < fingerTable[k].end){
					sendNtoN(nodeInfo, 2, "LOOKUP", *successor0);
					break;
				}
			}
		}
		node node_empty;
		node_empty.id = -1;
		return node_empty;
	}

}

/**
send node info to another node
1. In lookup function: ask other node lookup for the search request
2. In join: send the new node info to others to let other update
3. handle: forward node to others
*/
void sendNtoN(node nodeInfo[], int numberOfNode, char* message, node requestNode){
	printf("sendNtoN begin.\n");
	int serversocket, n;
	struct sockaddr_in serveradd, clientadd;
	char sendline[1000];
	char recvline[1000];

	while (1){
		serversocket = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&serveradd, sizeof(serveradd));
		serveradd.sin_family = AF_INET;
		serveradd.sin_addr.s_addr = inet_addr((&requestNode)->ip);
		serveradd.sin_port = htons((&requestNode)->port);

		if ((n = connect(serversocket, (struct sockaddr *) &serveradd,
				sizeof(serveradd))) < 0){
			printf("sendNtoN Error: %s\n", strerror(errno));
		}

		if (n >= 0){
			break;
		} 
	}

	strcpy(sendline, message);
	strcat(sendline, "\n");
	sendto(serversocket, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveradd, sizeof(serveradd));
	//printf("sent:%s\n", sendline);

	int i = 0;
	for (i = 0; i <= numberOfNode - 1; i++){
		sprintf(sendline, "%ld", (&nodeInfo[i])->id);
		strcat(sendline, "\n");
		sendto(serversocket, sendline, strlen(sendline), 0,
				(struct sockaddr *) &client_addr, sizeof(client_addr));
		//printf("sent:%s\n", sendline);

		strcpy(sendline, (&nodeInfo[i])->ip);
		strcat(sendline, "\n");
		sendto(serversocket, sendline, strlen(sendline), 0,
				(struct sockaddr *) &serveradd, sizeof(serveradd));
		//printf("sent:%s\n", sendline);

		sprintf(sendline, "%d", (&nodeInfo[i])->port);
		strcat(sendline, "\n");
		sendto(serversocket, sendline, strlen(sendline), 0,
				(struct sockaddr *) &client_addr, sizeof(client_addr));
		//printf("sent:%s\n", sendline);
	}
	close(serversocket);
}

/**
update finger table
*/
void updateFTable(long id1, long id2, int a){
	printf("UpdateFT begins.\n");
	int i;
	if (a == 1){
		long distanceNodeStart, distanceNodeId, distanceNodeNewId;
		if (id1 >= current->id)
			distanceNodeNewId = id1 - current->id;
		else
			distanceNodeNewId = KEY_SIZE - current->id + id1;

		for (i = 0; i <= FINGERTABLE_SIZE - 1; i++){
			distanceNodeStart = pow(2, i);
			if (fingerTable[i].id > current->id)
				distanceNodeId = fingerTable[i].id - current->id;
			else
				distanceNodeId = KEY_SIZE - current->id + fingerTable[i].id;
			if (distanceNodeNewId >= distanceNodeStart && distanceNodeNewId < distanceNodeId){
				fingerTable[i].id = id1;
			}
		}
	} else{
		for (i = 0; i <= FINGERTABLE_SIZE - 1; i++){
			if (fingerTable[i].id == id1){
				fingerTable[i].id = id2;
			}
		}
	}
}

/**
update finger table
*/
void insertFTable(long start, long nodeId){
	printf("insertFT begins.\n");
	int i;
	for (i = 0; i <= FINGERTABLE_SIZE - 1; i++){
		if (fingerTable[i].start == start){
			fingerTable[i].id = nodeId;
			break;
		}
	}
}
/**
*/
void sendStoN(char *msg, char strs[][1000], int size, node requestNode){
	printf("sendStoN begins.\n");
	struct sockaddr_in serveradd, clientadd;
	int serversocket, n;
	char sendline[1000];
	char recvline[1000];

	while (1){
		serversocket = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&serveradd, sizeof(serveradd));
		serveradd.sin_family = AF_INET;
		serveradd.sin_addr.s_addr = inet_addr((&requestNode)->ip);
		serveradd.sin_port = htons((&requestNode)->port);

		if ((n = connect(serversocket, (struct sockaddr *) &serveradd,
			sizeof(serveradd))) < 0){
			//printf("sendStoN error:%s\n",strerror(errno));
		}
		if (n >= 0){
			printf("sendStoN succ.\n");
		break;
		}
	}

	strcpy(sendline, msg);
	strcat(sendline, "\n");
	sendto(serversocket, sendline, strlen(sendline), 0,
		(struct sockaddr *) &serveradd, sizeof(serveradd));
		//printf("sent:%s\n", sendline);

	sprintf(sendline, "%d", size);
	strcat(sendline, "\n");
	sendto(serversocket, sendline, strlen(sendline), 0,
		(struct sockaddr *) &client_addr, sizeof(client_addr));
		//printf("sent:%s\n", sendline);

	int i = 0;
	for (i = 0; i <= size - 1; i++){

		strcpy(sendline, strs[i]);
		strcat(sendline, "\n");
		sendto(serversocket, sendline, strlen(sendline), 0,
			(struct sockaddr *) &serveradd, sizeof(serveradd));
			//printf("sent:%s\n", sendline);
	}
	close(serversocket);
}
/**
Different rules return different value. If 0, failed.
*/
int joinIn(node nodeNew, node *preToJoin, node *sucToJoin){
	printf("Join in begins.\n");
	if (current->id == predecessor0->id && current->id == successor0->id){
		*preToJoin = *current;
		*sucToJoin = *current;
		*predecessor0 = nodeNew;
		*successor0 = nodeNew;
		return 1;
	} 
	else if ((&nodeNew)->id > current->id && (&nodeNew)->id < successor0->id){

		*preToJoin = *current;
		*sucToJoin = *successor0;
		*successor0 = nodeNew;
		return 2;
	}
	else if ((&nodeNew)->id > current->id && predecessor0->id == successor0->id
			&& current->id > successor0->id){
		*preToJoin = *current;
		*sucToJoin = *successor0;
		*successor0 = nodeNew;
		return 2;
	} 
	else if ((&nodeNew)->id > current->id && (&nodeNew)->id > successor0->id
			&& predecessor0->id > successor0->id && predecessor0->id < current->id && successor0->id < current->id){

		*preToJoin = *current;
		*sucToJoin = *successor0;
		*successor0 = nodeNew;
		return 2;
	} 
	else if ((&nodeNew)->id < current->id && (&nodeNew)->id < successor0->id && predecessor0->id == successor0->id && current->id > successor0->id){
		*preToJoin = *current;
		*sucToJoin = *successor0;
		*successor0 = nodeNew;
		return 2;
	}
	else if ((&nodeNew)->id < current->id && (&nodeNew)->id < successor0->id && predecessor0->id > successor0->id && predecessor0->id < current->id && successor0->id < current->id){

		*preToJoin = *current;
		*sucToJoin = *successor0;
		*successor0 = nodeNew;
		return 2;
	}
	return 0;
}

// void keepAlive(){
// 	//send IS_ALIVE to the successor node
//	//need to deal with confirmation.
// }