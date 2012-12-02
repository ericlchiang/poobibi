//======================================//
//Server
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
#include <sys/wait.h>	
#include <signal.h>	
#include <vector>


using namespace std;


struct packet
{
	string sequence_num;
	string payLoad;
	string lastPacket;
};

void timeInit(timeval *timeout, int sec, int milliSec)
{
	// Set timeout value 
	timeout->tv_sec = sec;		//sec
	timeout->tv_usec = milliSec;	//milli-sec
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

string intToString(int num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}

vector<string> extractFileToVector(char* buf, size_t bytesRead)
{

	FILE *fp;
	int length;
	int sequenceNum = 0;
	char* buffer;
	char* buffer2;
	string buffer_str;
	size_t desiredPacketSize = 250;
	size_t bytesRead2;
	int lastPacket = 0;
	vector<string> packet_vector;
int totalRead=0;

	fp = fopen(buf, "rb");
	// obtain file size:
	fseek(fp, 0, SEEK_END);
	length = ftell(fp)+1;
	rewind(fp);

	while(1)
	{
		//Just make a buffer that's big enough
		buffer = new char[2000];
		buffer2 = new char[5];
		bzero(buffer, 2000);

		if(buffer == NULL) {fputs("Memory error",stderr); exit (2);}
		//Read Data

		bytesRead = fread(buffer, 1, desiredPacketSize, fp);
		totalRead +=bytesRead;
		if(bytesRead == 0)
		{
cout << "Stopped..." << endl;
			fclose(fp);
			break;
		}
		//convert char* to string for easy manipulation

cout << "Desired Bytes: " << desiredPacketSize << endl;
//cout << buffer << endl;
cout << "Bytes Read: " << bytesRead << endl;
		if(bytesRead != desiredPacketSize )	//Last packet
		{
			cout << "Last Packet!!!!" << endl;
			lastPacket = 1;
		
			for(size_t i = 0; i < bytesRead; i++)
				buffer_str += buffer[i];

		} else					//regular packets
		{	
			buffer_str = buffer;
			bytesRead2 = fread(buffer2, 1, 1, fp);
			if (bytesRead2 == 0)
				lastPacket = 1;
			else 
				fseek(fp, -1,  SEEK_CUR);
		}

		//buffer_str = {payload|%|%|sequenceNum|%|%|lastPacket'\0'}
		buffer_str += "|%|%|";
		buffer_str += intToString(sequenceNum);
		buffer_str += "|%|%|";;
		buffer_str += intToString(lastPacket);
		buffer_str += '\0';

		packet_vector.push_back(buffer_str);
		
		sequenceNum++;
		buffer_str = "";
		delete[] buffer;
		delete[] buffer2;
	}
cout << "read: " << totalRead << endl;
	return packet_vector;
}

void waitToRead(int sockfd, char* buf, int const MAXBUFLEN, struct sockaddr_storage &their_addr)
{
	int maxfdp, result;
	int const SERVER_TIME_OUT = 3; 	//Timeout after 3 secondsZ
	int numbytes;
	int bytesRead;
	int packetSize;
	fd_set rset;
	struct timeval timeout;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	bool fileOpened = false;
	string file_name ="";
	int sequenceNum = 0;
	string buf_str;
	vector<string> packet_vector;

	while(true)
	{
		//Initialize file_descriptors
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		timeInit(&timeout, SERVER_TIME_OUT, 0);

		maxfdp = sockfd + 1;	//For compatibility issues...
		if ((result = select(maxfdp, &rset, NULL, NULL, &timeout) )< 0)
		{
			perror("select failed");
			return;
		}
		if (result == 0)
		{
			addr_len = sizeof(their_addr);
			// timer expires
	 		cout << "TIME OUT!" << endl;
			time_t t = time(0);  // t is an integer type
    			cout << "Current Time: " << t << endl;
    
			cout << fileOpened << endl;
			if(fileOpened)
			{

				//Break if we have finished transmitting
				if(sequenceNum > packet_vector.size()-1)
					break;

				packetSize =strlen(packet_vector[sequenceNum].c_str());
			
				if((numbytes=
					sendto(sockfd, packet_vector[sequenceNum].c_str(), 
						packetSize, 0, (struct sockaddr *)&their_addr, addr_len)) < 0) 
				{
					perror("Send Failed!");
					exit(1);
				}
				cout << "Retransmitting packet " << sequenceNum << endl;
			}
			else
				cout << "File not opened yet." << endl;
		} 

		if (FD_ISSET(sockfd, &rset)) //If sockfd is ready
		{ 
			
			cout << "listener: waiting to recvfrom..." << endl;
	
			addr_len = sizeof(their_addr);
			if ((numbytes = 
				recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len))
				== -1) 
			{
				perror("recvfrom");
				exit(1);
			}

			//converts address from byte-form to text
			//cout << "listener: got packet from "
			//     << inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s)) 
			//     << endl;

			cout << "listener: packet is " << numbytes << " bytes long ";
			buf[numbytes] = '\0';
			cout << ", contains \"" << buf << "\"" << endl;
			buf_str = buf; 

			if (buf_str.find("|", 0, 1) != string::npos)
				sequenceNum = atoi(buf_str.substr(buf_str.find("|", 0, 1)+1).c_str())+1;
			else
				sequenceNum = 0;

			

			//If the sequence number on the packet received is NOT equal to currentPacketToSend+1
			
			//=======================//
			//Extracting file content
			//=======================//
			if(!fileOpened)
			{
				fileOpened = true;
		
				packet_vector = extractFileToVector(buf, bytesRead);

			}

			//=======================//
			//Sends packet to client
			//=======================//
			
			//Break if we have finished transmitting
			if(sequenceNum > packet_vector.size()-1)
				break;

			packetSize =strlen(packet_vector[sequenceNum].c_str());
			
			if((numbytes=
				sendto(sockfd, packet_vector[sequenceNum].c_str(), 
					packetSize, 0, (struct sockaddr *)&their_addr, addr_len)) < 0) 
			{
				perror("Send Failed!");
				exit(1);
			}
		}	
	}
}

int main(int argc, char *argv[])
{
	int const MAXBUFLEN = 2000;
	int myPort;	//User connects to this port
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
    	char ipstr[INET6_ADDRSTRLEN];

	if(argc != 2)
	{
		cerr << "Wrong number of arguments. ( ./sender 4990)" << endl;
		exit(1);	
	}

	myPort = atoi(argv[1]);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM; 	//Datagram socket
	hints.ai_flags = AI_PASSIVE; // use my IP (AI_PASSIVE  tells getaddrinfo() to assign the address of my local host to the socket structures)

	if ((rv = getaddrinfo(NULL,intToString(myPort).c_str(), &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		/*************Print Info********************/
		void *addr;
		string ipver;
		
		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if(p->ai_family == AF_INET) 
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
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);

		cout << " " << ipver << ": " << ipstr << endl;
	
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) 
		{
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
		{ 
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	waitToRead(sockfd, buf, MAXBUFLEN, their_addr);

	//** 

	close(sockfd);
	return 0;
}
