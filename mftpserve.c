//==================================================
// Choong Huh
// mftpserve.c
//
// mftpserve.c is the server side of the miniature FTP project.
// upon establishing connection with a client, a process is forked
// to handle the client commands. 
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>

#define MY_PORT_NUMBER 49999
#define MAX_BUFFER 100

char* return_message(int socketfd)
{
	char temp[1];
	char *out=malloc(sizeof(char)*200);
	int i=0;

	while((read(socketfd,temp,1))>0)
	{	
		if(temp[0]=='\n')
			{
				out[i]='\0';
				return out;
			}
		out[i]=temp[0];
		i++;
	}
	free(out);
	return NULL;
}

//=============================================
// getfilename( )
//

char* getfilename(char* path)
{
	char *last, *prev;
    struct stat file_info;

	if((access(path, R_OK))==0)	// Check file access
	{
		lstat(path, &file_info);

		if((S_ISREG(file_info.st_mode))!=0)
		{
			last = strtok(path, "/");

			while(last)
			{
				prev=last;		
				last=strtok(NULL,"/");
			}

			printf("%s\n", prev);
			return prev;
		}
		else if((S_ISDIR(file_info.st_mode))!=0)
		{
			printf("Directory Operation Not Permitted\n");
		}
	}

	return '\0';
}

//=============================================
// getfilename2( )
//

char* getfilename2(char* path)
{
	char *last, *prev;

			last = strtok(path, "/");

			while(last)
			{
				prev=last;		
				last=strtok(NULL,"/");
			}

			printf("%s\n", prev);
			return prev;

}


int setdc(int connectfd)
{
	int listenfd2, datafd;
	struct sockaddr_in servAddr, someAddr;
	char *address;

	listenfd2=socket(AF_INET,SOCK_STREAM,0);

			int	length=sizeof(servAddr);

				memset(&servAddr,0,sizeof(servAddr));
				servAddr.sin_family = AF_INET;
				servAddr.sin_port = htons(0);	
				servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

			if((bind(listenfd2,			// arguments for bind: ( sockfd, *addr, addrlen )
						(struct sockaddr *)&servAddr,
						sizeof(servAddr)))<0)
						{
							perror("[servAddr]bind");
							exit(1);
						}

			if((getsockname(listenfd2,(struct sockaddr *)&someAddr,&length))<0)
			{
				printf("getsockname failure\n");
				return -1;
			}

			length = sizeof(struct sockaddr_in);
			sprintf(address,"A%u\n",ntohs(someAddr.sin_port));
			printf("Imma give my client %s\n",address);
			write(connectfd,address,strlen(address));
			listen(listenfd2,1);

			if((datafd = accept(listenfd2, (struct sockaddr *)&someAddr,&length))<0)
			{
				perror("connection");
			}
		printf("Accepted a connection from client\n");

	return datafd;
}

void serverls(int datafd)
{
	int pid;
	if((pid=fork())==0)
	{
		dup2(datafd,1);
		execlp("ls","ls","-l",NULL);
	}
	wait(pid);	
}

void handle_g(int connectfd, int datafd, char *filename)
{
	int fd;
	int bytesread;
	char buffer[MAX_BUFFER];

	printf("Accepted G and filename: %s\n", filename);

	if((fd=open(filename,O_RDONLY,777))<0)
	{
		printf("[G]file open error\n");
		write(connectfd,"EFile open error: check permission or if directory\n",MAX_BUFFER);
		return;
	}

	write(connectfd,"A\n",2);						
	printf("Wrote A to client. Beginning Transfer\n");

	while((bytesread=read(fd,buffer,MAX_BUFFER))>0)
	{
		write(datafd,buffer,bytesread);
	}
	printf("Transfer of %s complete\n", filename);
	close(fd);
	return;
}

void handle_p(int connectfd, int datafd, char* filename)
{
	int fd;
	int bytesread;
	char buffer[MAX_BUFFER];

	printf("Accepted P and filename: %s\n", filename);

	if((fd=open(filename,O_EXCL|O_CREAT|O_WRONLY,777))<0)
	{
		printf("get->open error\n");
		write(connectfd,"EFile open error: could not create on server side\n",MAX_BUFFER);	
		return;
	}
		write(connectfd,"A\n",2);
		printf("Wrote A to client. Beginning Transfer\n");


	while((bytesread=read(datafd,buffer,MAX_BUFFER))>0)
	{
		write(fd, buffer, bytesread);
	}

	printf("Transfer of %s complete\n", filename);
	close(fd);
	return;	
}

int main(void)
{
	int listenfd, listenfd2, datafd;
	int n=0;
	int pid;

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("listen");
	}

	struct sockaddr_in servAddr;
	struct sockaddr_in dataAddr;
	
	char buffer[MAX_BUFFER];
	char clientcall[2];
	char *path;
	char *clientmessage;

// htons( ) converts hostshort from host byte order to network byte order
// htonl( ) converts hostlong from host byte order to network byte order
	
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(MY_PORT_NUMBER);	
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);	

	if((bind(listenfd,			// arguments for bind: ( sockfd, *addr, addrlen )
			(struct sockaddr *)&servAddr,
			sizeof(servAddr)))<0)
			{
				perror("bind");
				exit(1);
			}
			
	puts("gonna listen now");
	
	listen(listenfd, 4);		// set up a connection queue four level deep
// arguments for listen: (sockfd, backlog);
// backlog defines the max length the queue of pending connection for sockfd may grow.

	int connectfd;
	connectfd = socket(AF_INET, SOCK_STREAM,0);
	int length = sizeof(struct sockaddr_in);
	struct sockaddr_in clientAddr;

// Wait until a connection is established by a client.
// When that happens, a new descriptor for the connection is returned.
// Client address is written into the sockaddr structure (clientAddr).

// FROM accept() MAN PAGE
// it extracts the first connection request on the queue for the listening socket sockfd,
// creates a new connection socket, and returns a new fd referring to that socket.
//
while(1)
	{
	if((connectfd = accept(listenfd,
								(struct sockaddr *) &clientAddr,
								&length))<0)
		{
		perror("connect");
		}							

	if(fork())
	{}
	else
	{
		struct hostent* hostEntry;
		char* hostName;


// Uses DNS to convert a numerical Internet address to a host name.
	
		if((hostEntry = gethostbyaddr(&(clientAddr.sin_addr),
											sizeof(struct in_addr),
											AF_INET))==NULL)
			{
			herror("gethostbyaddr");
			}
												
		hostName = hostEntry->h_name;										
		//fork();
		printf("Client connected from %s\n",hostName);
		int i = 0;

		for(;;)
		{
			clientmessage=return_message(connectfd);

			if(clientmessage[0]=='C')		// RCD
			{
				if((chdir(clientmessage+1))<0)
					{
						printf("chdir is not working\n");
						write(connectfd,"E\n",2);
					}
				else
					{
						printf("chdir to path %s success\n",clientmessage+1);
						write(connectfd,"A\n",2);
					}
			}
			else if(clientmessage[0]=='Q')
				{
				printf("Client disconnected with a message %s\n", clientmessage);
				close(connectfd);
				return 0;
				}
			else if(clientmessage[0]=='D')
				{
//				write(connectfd,"E\n",2);

				printf("Data connection request was sent\n");
				if((datafd = setdc(connectfd))<0)
				{
					printf("fatal error-setdc failure\n");
					return 0;
				}

				clientmessage=return_message(connectfd);

					if(clientmessage[0]=='L')		// RLS
					{
						printf("L\n");
						write(connectfd,"A\n",2);
						serverls(datafd);
						close(datafd);
					}

					else if(clientmessage[0]=='G')
					{
						printf("G\n");
	
						path=getfilename(clientmessage);

						if(path)
						{
							write(connectfd,"EError: Check File Permission Or If Directory\n",50);
						}
						else
						{
							path=getfilename2(clientmessage);
							handle_g(connectfd,datafd,path+1);	
							close(datafd);
						}

					}
					else if(clientmessage[0]=='P')
					{
						printf("P\n");
						path=getfilename2(clientmessage);

						handle_p(connectfd,datafd,path+1);
						close(datafd);
					}
				}	
							
				else
				{
				printf("idk what youre saying dude\n");
				}
		}
		return 0;

//	time_t thing = time(NULL);
//	timestring = ctime(&thing);	

//	write(connectfd,timestring ,MAX_BUFFER);
		}
	}
}
