//======================================//
//Client
//======================================//


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

string intToString(int num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}

void timeInit(timeval *timeout, int sec, int milliSec)
{
	// Set timeout value 
	timeout->tv_sec = sec;		//sec
	timeout->tv_usec = milliSec;	//milli-sec
}


void waitToRead(int sockfd, char* buf, int const MAXBUFLEN, struct sockaddr **ai_addr, ofstream &outfile, int Plost, int Pcorrupt)
{
	int maxfdp, result;
	int const SERVER_TIME_OUT = 3; 	//Timeout after 3 secondsZ
	int numbytes;
	fd_set rset;
	int packetSize;
	string sequenceNum;
	string lastPacket;
	string payload;
	size_t firstDelimPos, secondDelimPos;
	size_t ai_addrlen;
	struct timeval timeout;
	string buf_str;
	string ACKMessage;
	char* deliminator = new char[5];
	strcpy(deliminator, "|%|%|");
	int deliminatorLen = strlen(deliminator);

	while(true)
	{
		//Initialize file_descriptors
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		timeInit(&timeout, SERVER_TIME_OUT, 0);


		//maxfdp = max(fileno(fp), sockfd) + 1;	//For compatibility issues...
		maxfdp = sockfd + 1;	//For compatibility issues...
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
			bzero(buf, MAXBUFLEN);
			//=======================//
			//Receives from the server
			//=======================//
			
			ai_addrlen = sizeof(*ai_addr);
			if ((numbytes = 
				recvfrom(sockfd, buf, MAXBUFLEN-1, 0, 
					 *ai_addr, 
					 (socklen_t *) &(ai_addrlen) ) ) < 0) 
			{
				perror("Failed to receive from server");
				exit(1);
			}
			buf_str = buf;
			firstDelimPos = buf_str.find(deliminator, 0, deliminatorLen);
			secondDelimPos = buf_str.find(deliminator, firstDelimPos+5, deliminatorLen);
			payload = buf_str.substr(0, firstDelimPos);
			sequenceNum = buf_str.substr(firstDelimPos+5, secondDelimPos-firstDelimPos-5);
			lastPacket = buf_str.substr(secondDelimPos+5, 1);

			cout << "payload: " << payload << endl << endl;
			cout << "Sequence Num: " << sequenceNum << endl << endl;
			cout << "Last packet? " << lastPacket << endl;


			outfile << payload;
			
			if(atoi(lastPacket.c_str()) == 1)
				outfile.close();

			//cout << "delim Length: " <<  deliminatorLen << endl;
			//cout << "first: " <<  firstDelimPos << endl;
			//cout << "second: " << secondDelimPos << endl;


			//cout << "IT IS: " << buf_str << endl;
	
			//=======================//
			//Sends ACK to server
			//=======================//

			ACKMessage = "ACK|" ;
			ACKMessage += sequenceNum;
			//packetSize = strlen(ACKMessage.c_str());
			

cout << "Buff string is: " << buf_str << endl;
cout << "ACK Message is: " << ACKMessage << endl;

cout << "Sendto Info: " <<  (struct sockaddr *)ai_addr << endl;
cout <<(socklen_t) ai_addrlen<< endl;
cout << sockfd << endl;

			if ((numbytes = sendto(sockfd, ACKMessage.c_str(), strlen(ACKMessage.c_str()), 0, *ai_addr,
					       ai_addrlen)) == -1) 
			{
				perror("Failed to send to server");
				exit(1);
			}
	
		}

	}
}

//TODO: use recvfrom(...) with combination with select(...) to talk back and forth b/t server and client
int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	int serverPort;		//clients connect to this server port
    	char ipstr[INET6_ADDRSTRLEN];
	int const MAXBUFLEN = 2000;
	char buf[MAXBUFLEN];
	int Plost, Pcorrupt;
	ofstream outfile;
	

	if (argc != 6) 
	{
		cerr << "Invalid Arguments. (./receiver 0.0.0.0 4950 myFile.txt 50 0)" << endl;
		exit(1);
	}

	serverPort = atoi(argv[2]);	
	Plost = atoi(argv[4]);
	Pcorrupt = atoi(argv[5]);

	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

    	// "getaddrinfo" will do the DNS lookup
	if ((rv = getaddrinfo(argv[1], intToString(serverPort).c_str(), &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{

		/*************Print Info********************/
		void *addr;
		string ipver;
		
		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) 
		{ // IPv4
		    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		    addr = &(ipv4->sin_addr);
		    ipver = "IPv4";
		} 
		else 
		{ // IPv6
		    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		    addr = &(ipv6->sin6_addr);
		    ipver = "IPv6";
		}
		
		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		cout << " " << ipver << ": " << ipstr << endl;
		/************************************/
		
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) 
	{
		perror("talker: failed to bind socket\n");
		return 2;
	}
cout << "Sendto Info: " <<  p->ai_addr << endl;
cout << p->ai_addrlen << endl;
cout << sockfd << endl;
	
	if ((numbytes = sendto(sockfd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen)) == -1) 
	{
		perror("talker: sendto");
		exit(1);
	}

	outfile.open(argv[3]);


	freeaddrinfo(servinfo);

	cout << "talker: sent " << numbytes << " bytes to " << argv[1] << endl;

	cout << "Waiting to receive from server..." << endl;

	//waitToRead(sockfd, buf, MAXBUFLEN, &(p->ai_addr), p->ai_addrlen, outfile);
	waitToRead(sockfd, buf, MAXBUFLEN, &(p->ai_addr), outfile, Plost, Pcorrupt);
	//**


	close(sockfd);
	return 0;
}