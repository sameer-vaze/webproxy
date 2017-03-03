#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>


#define MAX 100						//set maximum number of requests 
#define MAXBUF 1000			
#define BUFSIZE 1000 					//to send via socket

int sock, serv[MAX], slot;
struct sockaddr_in clientaddr;
int timeout;
FILE *f;

typedef struct ll 							//node for linked list to store formats supported
{
	unsigned long hash;
	int hour;
	int minutes;
	int seconds;
	int available;
	struct ll *next;
}node;

node *head = NULL;							//global variable to point to first item in linked list
node *current = NULL;						//global variable to traverse through the linked list

void updateLL()
{
	node *start;
	start = head;
	time_t now1;
	struct tm *tm1;
	
	now1 = time(0);
	if ((tm1 = localtime (&now1)) == NULL)
	{
		printf ("Error extracting time stuff\n");
	}
	printf ("Time during update is: %d:%d:%d\n", tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	
	while(start != NULL)
	{
		if(tm1->tm_hour = start->hour)
		{
			if((tm1->tm_min*60 + tm1->tm_sec)-(start->minutes*60 + start->seconds) > timeout)
			{
				start->available = 0;
				printf("Hashed file %lu is no longer available locally!\n", start->hash);
			}
		}
		start = start->next;
	}
}

void insertToLL(unsigned long a, int b, int c, int d, int e)				//function that adds items to linked list
{
	int l;
	node *link = (node*) malloc(sizeof(node));
	link->hash = a;
	link->hour = b;
	link->minutes = c;
	link->seconds = d;
	link->available = e;
	if(head == NULL)							//if element added is first element
	{
		head = link;
		link->next = NULL;
		current = link;
	}
	else										//if element added is any subsequent element
	{
		current->next = link;
		link->next = NULL;
		current = current->next;
	}
}

unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
	{
		hash = ((hash << 5) + hash) + c;
	}
    return hash;
}

void serverStart(int port)								//funtion that sets address, family, port number to socket. It creates socket, binds socket and listens to any incoming requests
{
	struct sockaddr_in sin;
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;							//set family as the internet family (AF_INET)
	sin.sin_addr.s_addr = INADDR_ANY;					//set any address as the address for the socket
	sin.sin_port = htons(port);							//set the port number to the socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)	//open socket  and check if error occurs
	{
		printf("Error in creating socket \n");
		exit(1);
	}

	// printf("sock is %d\n", sock);

	int optval = 1;
  	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));		//prevents the "already in use" issue
	printf("Socket created\n");
	if ((bind(sock, (struct sockaddr *)&sin, sizeof(sin)))	 < 0)		//bind socket and check if error occurs
	{
		printf("%d\n", bind(sock, (struct sockaddr *)&sin, sizeof(sin)));
		printf("Error in binding socket\n");
		perror("Bind error: ");
		exit(1);
	}
	printf("Socket bound\n");	
	if(listen(sock, 4) != 0)							//listen for incoming requests on the socket we have created
	{
		printf("Error in listen\n");
		exit(1);
	}
	printf("Listening for connections\n");	
}

void parseRequest(int n)								//function that parses the requests and returns the necessary pages
{
	int status, serv_port;
	struct addrinfo hints;
	struct addrinfo *servinfo;int rcv, bytes, char_index, x, content_length, soc;
	struct sockaddr_in addr_server;
	FILE *fd;
	char msg[MAXBUF], content_type[] = "Content-Type:<>\r\n", invalid[500], httptext[50], incorrectmethod[100],*command, *httpversion, *filename, *host_line, *p_no = NULL, *msg_body, extension[10], path[MAXBUF], BUFFER[BUFSIZE];
	char *temp, *host_dir, *port_string, *hash_string;
	char send_content[MAXBUF], recv_content[MAXBUF], local_store[MAXBUF];
	unsigned long hashval;
	time_t now;
	struct tm *tm;
	char copy[MAXBUF];
	
	bzero(msg, sizeof(msg));
	rcv = recv(serv[n], msg, MAXBUF, 0);
	// printf("Strlen is %d\n", (int)strlen(msg));
	bzero(copy, sizeof(copy));
	strcpy(copy, msg);
	if(rcv < 0)										//if there is an error in sending the request from the client side
	{
		printf("Error in recv()\n");
	}
	else if(rcv == 0)								//if there are no requests received from client
	{
		printf("No client\n");
	}
	else											//if a request message is received
	{
		printf("%s\n", msg);
		command = strtok(msg, " \t\n");				//extract the method (GET, PUT, POST, MOVE, COPY, DEFINE etc)
		printf("Value of command now is %s\n", command);
		if(strncmp(command, "GET\0", 4) == 0)		//if the method is GET
		{
			filename = strtok(NULL, " \t");			//extract filename
			// printf("filename is %s\n", filename);
			hashval = hash(filename);			
			hash_string = (char *)malloc(50);
			sprintf(hash_string, "%lu", hashval);
			// printf("hash is %lu\n", hashval);
			// printf("hash_string is %s\n", hash_string);
			httpversion = strtok(NULL, " \t\n");	//extract the HTTP version
			// printf("httpversion is %s\n", httpversion);
			msg_body = strtok(NULL, "\0");
			// printf("msg_body is \n\n%s\n", msg_body);
			temp = (char *)malloc(strlen(filename));
			strcpy(temp, filename);
			// printf("temp is %s\n", temp);

			if(strncmp(temp, "http://",7) == 0)
			{
				strtok(temp, "//");
				host_line = strtok(NULL, "/");
				host_dir = strtok(NULL, ":\0");
			}
			else
			{
				host_line = strtok(temp, "/");
				host_dir = strtok(NULL, ":\0");
			}
			// printf("host_line is %s\n", host_line);
			// printf("host_dir is %s\n", host_dir);

			if(strncmp(filename, "http://",7) == 0)
			{	
				filename = filename + 7;
				while(*filename != 0)
				{
					if(*filename == ':')
					{
						p_no = filename + 1;
					}
					filename++;
				}
			}
			else
			{
				while(*filename != 0)
				{
					if(*filename == ':')
					{
						p_no = filename + 1;
					}
					filename++;
				}				
			}

			if(p_no == NULL)
			{
				serv_port = 80;
			}
			else
			{
				serv_port = atoi(p_no);
			}

			port_string = (char *)malloc(5);
			sprintf(port_string, "%d", serv_port);
			// printf("port_string is %s\n", port_string);
			
			node *check;
			check = head;
			int avail;

			while(check != NULL)
			{
				if(check->hash == hashval)
				{
					avail = check->available;
				}
				check = check->next;
			}

			if(access(hash_string, F_OK) == 0 && (avail == 1))
			{
				int x;
				if((x = open(hash_string, O_RDONLY)) != -1)
				{
					while(bytes = read(x, send_content, MAXBUF) > 0)
					{						
						printf("strlen is %d\n", (int)strlen(send_content));
						printf("send_content %s\n", send_content);
						send(serv[n], send_content, strlen(send_content), 0);
						bzero(send_content, MAXBUF);
					}
					close(x);
				}
			}
			else
			{
				if((soc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				{
					perror("Error opening socket");
				}

				bzero((char *) &addr_server, sizeof(addr_server));
				addr_server.sin_family = AF_INET;
				addr_server.sin_port = htons(serv_port);

				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_flags = AI_PASSIVE;
				
				if ((status = getaddrinfo(host_line, port_string, &hints, &servinfo)) != 0)
				{
					fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
					exit(1);
				}

				char * ip = (char *)malloc(INET_ADDRSTRLEN);
				inet_ntop(AF_INET, &((struct sockaddr_in *)servinfo->ai_addr)->sin_addr, ip, INET_ADDRSTRLEN);
				printf("ip: %s\n",ip);

				
				addr_server.sin_addr.s_addr = inet_addr(ip);

				if (connect(soc, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0)
				{
					perror("Error connecting");
				}
		
				if(host_dir != NULL)
				{
					sprintf(send_content, "GET http://%s/%s %s\r\n%s\r\n\r\n", host_line, host_dir, httpversion, msg_body);
					// printf("SEND CONTENT is \n%s\n", send_content);				
				}
				else
				{
					sprintf(send_content, "GET http://%s %s\r\n%s\r\n\r\n", host_line, httpversion, msg_body);
					// printf("SEND CONTENT is \n%s\n", send_content);				
				}

				// send(soc, send_content, strlen(send_content), 0);
				send(soc, copy, strlen(copy), 0);

				printf("sock is %d\n", sock);

				int i, b;
				f = fopen(hash_string, "w");
				while((i = recv(soc, recv_content, MAXBUF, 0)) > 0)
				{
					printf("recv_content %s\n", recv_content);
					printf("i %d\n", i);
					int b = send(serv[n], recv_content, i, 0);
					int m = fwrite(recv_content, i, 1, f);
					printf("m %d\n", m);
					// perror("issue");
					// printf("b is %d\n", b);
					bzero(recv_content, MAXBUF);
				}
				fclose(f);
				
				now = time(0);
				if ((tm = localtime (&now)) == NULL)
				{
					printf ("Error extracting time stuff\n");
				}
				printf ("%d:%d:%d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
				insertToLL(hashval, tm->tm_hour, tm->tm_min, tm->tm_sec, 1);
				updateLL();
			}
		}
		else					//handle incorrect method name
		{
			printf("Command %s has not been implemented. Only GET requests are being handled\n", command);
			exit(1);
		}
		pthread_exit(NULL);
	}
}

void main(int argc, char *argv[])
{
	int port = atoi(argv[1]);
	timeout = atoi(argv[2]);
	int len;
	pthread_t tid;
	
	for (int n = 0; n < MAX; n++)								//set all new socket values as -1
	{
		serv[n] = -1;
	}

	serverStart(port);

	while(1)
	{
		len = sizeof(clientaddr);
		serv[slot] = accept(sock, (struct sockaddr *)&clientaddr, &len);			//create new socket when new request is received
		printf("Value of slot is: %d\n", slot);
		if (serv[slot]<0)									//if the new socket is not created
		{
			printf("Error in accept\n");
		}
		else													//call fork
		{
			pthread_create(&tid, NULL, parseRequest, slot);
			pthread_join(tid, NULL);
			/*if(fork() == 0)										//when forked successfully
			{
				parseRequest(slot);								//set new socket descriptor to array serv at index number slot
				exit(1);
			}*/
		}
		close(serv[slot]);
		
		while (serv[slot]!=-1) 								//increment to next available slot
		{
			slot = (slot+1)%MAX;
		}

		updateLL();
	}
}
