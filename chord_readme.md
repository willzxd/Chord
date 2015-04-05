#CHORD README

##How to compile and run
compile:

```
$ make all
```

create a new chord ring:

```
$ make all
$ ./chord [port]
```
join in an old chord ring:

```
$ make all
$ ./chord [port] [server ip address] [server port]
```


##Introduction
Every node began with create a new ring or join in an old ring.

Firstly, program will get the information of local machine. Secondly, it will start to create or join. It will establish a process to setup a port to listen and accept requests, a process to detect whether other node is alive, and a process to show the user interface. Requests include the message that help other node joins in the ring, lookup file position, forward lookup and so on.

This program deal with the real ip address. It can run on Ubuntu 14 successfully. If you want to run on the OS X system. You should change some arguments. For example, you need to change "eth0" to "en0" in CurrentInfo Function.

##Main Fuction

> Creat a new ring

void createRing(char*argv);

>main setupServer, listen and accept

int setupServer(char*argv);

>handle connection:

void handle(int sock);

>join an old ring

void join(char**argv);

>get node id

long getId(char *str);

>print node info

void printNode(node nodeToPrint);

>request other nodes to find the position

void sendNtoN(node nodeInfo[], int numberOfNode, char* message, node requestNode);

>strings communication

void sendStoN(char *msg, char strs[][1000], int size, node requestNode);

>update finger table

void updateFTable(long id1, long id2, int a);

>update finger table

void insertFTable(long start, long nodeId);

>new node join

int joinIn(node nodeNew, node *preToJoin, node *sucToJoin);

>uersinterface for users

void ui(void);

>init finger table

void initFTable(void);

>like stol in c++

long stringToLong();

>read a line every time 

int readaline(int socket, char *bufferpt, size_t length);

>find a key (or node) position

node lookup(long key, node node_source);

>node quits

void quit();

###Acknowledge
1. Sha1 library from Wei Dai and other contributors.(http://fossies.org/linux/liboauth/src/sha1.c)
2. Got help from Yuzhang Han (sha1 modify, readaline function & Update finger table).
