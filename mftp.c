//==================================================
// Choong Huh
// mftp.c
// mftp.c is the client side of the miniature FTP project.
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

DIR *dir;

//=============================================
// getconnection( )
//
// Contains most of the steps necessary to make a socket connection.
// This function is used to return connection to sockets outside.

int getconnection(int port, char* path)
{
	int socketfd;

	if((socketfd = socket( AF_INET, SOCK_STREAM, 0))<0)
	{
				perror("Socket [socketfd] Init.");
	}

	struct sockaddr_in 		servAddr;
	struct hostent* 		hostEntry;
	struct in_addr**		pptr;

	
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if((hostEntry = gethostbyname(path))==NULL)
	{
		herror("gethostbyname");		
	}	
	
	pptr = (struct in_addr **) hostEntry->h_addr_list;
	memcpy(&servAddr.sin_addr, *pptr, sizeof(struct in_addr));
	
	printf("attempting to connect\n");
	
	if(connect(socketfd,
				 (struct sockaddr *) &servAddr,
				 sizeof(servAddr))<0)
	{
		perror("connect");
		return -1;
	}

	return socketfd;		// Connection made
}

//=============================================
// return_message( )
//
// accepts a socket as argument. This function is called after client sends
// server a command. The message from the server is accepted and returned
// to the caller. \n character is replaced with string terminator.

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
// setupdc( )
//
// A separate socket connection is established in order to
// carry out a data connection e.g. for downloading a file.
// 

int setupdc(int socketfd, char* path)
{
	int datafd;
	int serverport;
	char* servermessage;

	write(socketfd,"D\n",2);

	servermessage=return_message(socketfd);

	if(servermessage[0]=='E')
	{
		printf("%s\n",servermessage+1);
		return;
	}
	else if(servermessage[0]=='A')
	{
		serverport=atoi(servermessage+1);
		datafd = getconnection(serverport,path);

		return datafd;
	}
	else
	{
		printf("setupdc failed\n");
		return;
	}

}

//=============================================
// rls( )
//
// remote ls call function
//

void rls(int datafd)
{
	int pid;
	printf("executing rls on the server\n");
	if((pid=fork())==0)
	{
		dup2(datafd,0);
		execlp("more","more","-20",NULL);
	}
	wait(pid);
}

//=============================================
// get( )
//
// GET function that acquires a file from the server
//

void get(int datafd, char* filename)
{
	int fd;
	int bytesread;
	char buffer[MAX_BUFFER];

		if((fd=open(filename,O_EXCL|O_CREAT|O_WRONLY,777))<0)
		{
			printf("get->open error from client side\n");
			return;
		}
		while((bytesread=read(datafd,buffer,MAX_BUFFER))>0)
		{
			write(fd, buffer, bytesread);
		}

		close(fd);
		return;

}

//=============================================
// put( )
//
// put function to upload file from client to server
//

void put(int datafd, char* filename)
{
	int bytesread;
	int fd;
	char buffer[MAX_BUFFER]; //?

	dir=opendir(filename);

		if(dir!=NULL)
		{
			printf("Cannot PUT a directory\n");
			return;	
		}

		if((fd=open(filename,O_RDONLY,777))<0) //CHECK THIS FOR SURE
		{
			printf("put->open error from client side\n");
			return;
		}
		// WRITING BLOCK
		while((bytesread=read(fd,buffer,MAX_BUFFER))>0)
		{
			write(datafd,buffer,bytesread);
		}
		close(fd);
		return;

}

//=============================================
// show( )
//
// SHOW function that displays the content of the 
// requested file
//

void show(int datafd)		// CHECK THIS TOO
{
	int pid;
	int bytesread;
	char buffer[MAX_BUFFER];
	
	if((pid=fork())==0)
	{
		while((bytesread=read(datafd,buffer,MAX_BUFFER))>0)
		{
			dup2(datafd,0);
			execlp("more","more","-20",NULL);
		}
		return;
	}
	wait(pid);
}

//=============================================
// getfilename( )
//
// Function to check permission status of requested files
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
	else
		printf("Unable To Open File\n");
	return '\0';
}

//=============================================
// getfilename( )
//
// For efficient execution, getfilename2 is used when
// it is known that the requested file checked permission
// and is indeed an existing FILE, not a dir
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

//=============================================
// main( )
//

int main(int argc, char *argv[])
{
// Make an Internet socket using TCP protocol
	
	int socketfd, datafd;
	int slength;
	int i,n;

	char* servermessage;
	char* filename;
	int serverport;

	int testfd;

	char call[MAX_BUFFER];
	char* address = malloc(sizeof(char) * MAX_BUFFER);

	if(argc<2)
	{
		printf("Usage: daytime hostname\n");
		printf("Program Exits\n");
		return 0;		
	}
	
	if((socketfd = getconnection(MY_PORT_NUMBER, argv[1]))<0)
		return 0;
	
	printf("connection established\n");
	printf("enter help for help\n");
	char* message;

	for(;;)
	{
		printf("MFTP>>  ");		// Prompt
		fgets(call,MAX_BUFFER,stdin);

		slength=strlen(call);
		call[slength-1]='\0';

		slength=strlen(call);		

			if(strncmp(call,"cd",2)==0)		//cd
			{			
				printf("cd\n");

				if(slength<4)
				{
					printf("Usage: cd <pathname>\n");
				}

				else if(!isspace(call[2]))
				{
					printf("give a space between cd and pathname\n");
				}
				else
				{
					for(i=0;i<slength-3;i++)
						address[i]=call[i+3];

					address[i]='\0';
					printf("pathname:%s\n",address);
					if(chdir(address)<0)
					{
						printf("chdir failure: check pathname\n");
					}
				}
			}										// end cd
			else if(strncmp(call,"rcd",3)==0)	//rcd
			{
				fflush(stdout);
				write(socketfd,"C",1);
				write(socketfd,call+4,strlen(call+4)); // assuming address ends with \n
				write(socketfd, "\n", 1);

				servermessage=return_message(socketfd);

				if(servermessage[0]=='E')
					printf("RCD error:%s\n",servermessage+1);
				else if(servermessage[0]=='A')
					printf("RCD to %s successful\n", call+4);
				
			}
			else if(strcmp(call,"ls")==0)	//ls
			{
				printf("ls\n");
				system("ls -l");
			}
			else if(strncmp(call,"rls",3)==0)	//rls
			{
				datafd = setupdc(socketfd,argv[1]);
				write(socketfd,"L\n",2);

				servermessage = return_message(socketfd);

				if(servermessage[0]=='E')
				{
					printf("%s\n",servermessage+1);
				}
				else
				{
					rls(datafd);
				}
				close(datafd);
				//printf("rls\n");
			}
			else if(strncmp(call,"get",3)==0)	//get
			{
				datafd = setupdc(socketfd,argv[1]);
				filename=getfilename2(call+4);

				write(socketfd,"G",1);
				write(socketfd,filename,strlen(filename));
				write(socketfd,"\n",1);
				servermessage = return_message(socketfd);

				if(servermessage[0]=='E')
				{
					printf("%s\n", servermessage+1);
				}
				else
				{
					get(datafd,filename);
				}
				close(datafd);
			}
			else if(strncmp(call,"show",4)==0)	//show
			{
				filename=getfilename2(call+5);


					datafd = setupdc(socketfd,argv[1]);
			
					write(socketfd,"G",1);
					write(socketfd,filename,strlen(filename));
					write(socketfd,"\n",1);
					
					servermessage = return_message(socketfd);
					
					if(servermessage[0]=='E')
					{
						printf("%s\n",servermessage+1);
					}
					else
					{
						show(datafd);
					}
					close(datafd);				
			}
			else if(strncmp(call,"put",3)==0)	//put
			{
				filename=getfilename(call+4);
				if(!filename)
					printf("Error: Check File Permission Or If Directory\n");
				else
				{
					datafd = setupdc(socketfd,argv[1]);
				
					write(socketfd,"P",1);
					write(socketfd,filename,strlen(filename));
					write(socketfd,"\n",1);
					
					servermessage = return_message(socketfd);
					
					if(servermessage[0]=='E')
					{
						printf("%s\n",servermessage+1);
					}
					else
					{
						put(datafd,filename);
					}
					close(datafd);
				}
			}
			else if(strcmp(call,"exit")==0)	//exit
			{
				printf("exit\n");
				write(socketfd,"Q\n",2);
				return 0;
			}
			else if(strcmp(call,"help")==0)
			{
				printf("\nAvailable user commands are as below:\n");
				printf("cd \t<pathname>   to change client side directory\n");
				printf("rcd \t<pathname>   to change server side directory\n");
				printf("ls \tto execute ls -l inside the client side directory\n");
				printf("rls \tto execute ls -l inside the server side directory\n");
				printf("get \t<pathname>   to move a file from server side to client\n");
				printf("show \t<pathname>   to retrieve a file from server and display content\n");
				printf("put \t<pathname>   to move a file from client side to server\n");
				printf("exit \tto exit program\n");
			}
			else		// Invalid input
			{
			printf("invalid input\n");
			}
	}
}
