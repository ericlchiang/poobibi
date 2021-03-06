
//===========================================//
//CLIENT
//===========================================//

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

void error(char *msg)
{
    perror(msg);
    exit(0);
}


//=======================================================//
//Client will be sending requests to the server, requests for certain files
//=======================================================//
int main(int argc, char *argv[])
{
	int senderPortnum;
	string senderHostname, filename;
	
	if (argc < 4) 
	{
         	fprintf(stderr,"ERROR, missing parameter\n");
         	exit(1);
    	}

	senderHostname = argv[1];
	senderPortnum = atoi(argv[2]);	//Extracting port number
	filename = argv[3];

cout << "Host name is: " << senderHostname << "\n";
cout << "Portnum is: " << senderPortnum << "\n";
cout << "filename: " << filename << "\n";


    
    return 0;
}































//===========================================//
//SERVER
//===========================================//


#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	
#include <signal.h>	
#include <string>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <vector>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct packet
{
	int sequence_num;
	vector<unsigned char> payLoad;
};

void timeInit(timeval *timeout, int sec, int milliSec)
{
	// Set timeout value 
	timeout->tv_sec = sec;		//sec
	timeout->tv_usec = milliSec;	//milli-sec
}

/*

typedef struct fd_set {
  u_int  fd_count;
  SOCKET fd_array[FD_SETSIZE];
} fd_set;
*/
void waittoread ( int sockfd)
{
	int maxfdp, result, length;
	int const SERVER_TIME_OUT = 3; 	//Timeout after 3 secondsZ
	fd_set rset;
	struct timeval timeout;
	ifstream is;
	string file_name ="";
	char* buffer = "";

	
	
	while(true)
	{
		//Initialize file_descriptors
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		timeInit(&timeout, SERVER_TIME_OUT, 0);


		//maxfdp = max(fileno(fp), sockfd) + 1;	//For compatibility issues...
		maxfdp =  1;	//For compatibility issues...
		if ((result = select(maxfdp, &rset, NULL, NULL, &timeout) )< 0)
		{
			perror("select failed");
			return;
		}
		if (result == 0)
		{
			// timer expires
	 		perror("Timer Expired");
		} 

		if (FD_ISSET(sockfd, &rset)) 
		{ 
			// socket is readable 
			getline(cin, file_name);
			
			is.open(file_name.c_str(), ifstream::in);

  			//Find out length of file
			is.seekg(0, ios::end);
  			length = is.tellg();
  			is.seekg (0, ios::beg);

			buffer = new char[length];
			//Read Data
			is.read (buffer, length);

			cout << buffer << endl;

			is.close();
			delete[] buffer;
		}

/*
		if (FD_ISSET(sockfd, &rset)) 
		{ 
		}
		if(FD_ISSET(fileno(fp), &rset)) 
		{
			//input is readable 
			if(fgets(sendline, MAXLINE, fp) == NULL)
				return; // all done 
			writen(sockfd, sendline, strlen(sendline));
		}
*/
	
	}
}



//Must acts as a server to send files to client whenever a request comes in
int main(int argc, char *argv[])
{
	int portNum = 0;
	//TODO: We need to read in the requested_file_name and port_number of sender from command line
	if (argc < 2) 
	{
         	fprintf(stderr,"ERROR, no port provided\n");
         	exit(1);
    	}

	portNum = atoi(argv[1]);	//Extracting port number
	
	waittoread( 0);	//Takes content of a.txt and print to cout


     return 0; 
}



















