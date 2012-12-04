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
#include <fcntl.h>


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
    timeout->tv_sec = sec;        //sec
    timeout->tv_usec = milliSec;    //milli-sec
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

void formatPacket(char* packet, size_t currentSize, int sequenceNumber, int lastPacket)
{
    char* buf;
    size_t counter = 0;

    packet[currentSize] = '|';
    currentSize++;

    buf = new char(50);
    sprintf(buf, "%d", sequenceNumber);

    while (buf[counter] != '\0')
    {
        packet[currentSize] = buf[counter];
        currentSize++;
        counter++;
    }

    packet[currentSize] = '|';
    currentSize++;

    sprintf(buf, "%d", lastPacket);
    packet[currentSize] = buf[0];
    currentSize++;
    
    packet[currentSize] = '|';
    currentSize++;
    packet[currentSize] = '\0';
    currentSize++;
}

vector<char*> extractFileToVector(char* buf)
{
    //pos_type to store the size of the file
    ifstream::pos_type size;
    //Buffer to read entire contents of file into
    char* memblock;
    //Vector of vectors
    vector<char*> packetVector;

    size_t totalBytesRead = 0;
    size_t numBytesRead = 0;
    size_t desiredPacketSize = 1000;
    int sequenceNumber = 0;
    int lastPacket = 0;

    char* packetBuffer;

    //Open file stream
    ifstream file (buf, ios::in|ios::binary|ios::ate);

    //Read entire file into memblock
    if (file.is_open())
    {
        size = file.tellg();
        memblock = new char [size];
        file.seekg (0, ios::beg);
        file.read (memblock, size);
        file.close();
    }
    else {
        cout << "Unable to open file" << endl;
        return packetVector;
    }    

    //Read the entire file into packets, one at a time
    while (totalBytesRead < size)
    {
        packetBuffer = new char [2000];
        memset((void*)packetBuffer, '0', 2000);
        numBytesRead = 0;
        //If this is the last packet
        if (totalBytesRead + desiredPacketSize > size)
        {
            lastPacket = 1;
            while (numBytesRead + totalBytesRead < size)
            {
                packetBuffer[numBytesRead] = memblock[totalBytesRead+numBytesRead];
                numBytesRead++;
            }
        } else {
            while (numBytesRead < desiredPacketSize)
            {
                packetBuffer[numBytesRead] = memblock[totalBytesRead+numBytesRead];
                numBytesRead++;
            }
        }
        totalBytesRead += numBytesRead;
        formatPacket(packetBuffer, numBytesRead, sequenceNumber, lastPacket);

        packetVector.push_back(packetBuffer);
        
        sequenceNumber++;
    }
   
    delete[] memblock;
    return packetVector;
}

void waitToRead(int sockfd, char* buf, int const MAXBUFLEN, struct sockaddr_storage &their_addr, int cwnd, int Plost, int Pcorrupt)
{
    int maxfdp, result;
    int const SERVER_TIME_OUT = 3;     //Timeout after 3 secondsZ
    int numbytes;
    int packetSize;
    fd_set rset;
    struct timeval timeout;
    socklen_t addr_len;
    bool fileOpened = false;
    string file_name ="";
    int sequenceNum = 0;
    int currentAck = 0;
    string buf_str;
    vector<char*> packet_vector;
    int temprand1, temprand2;
    int flags;

    while(true)
    {
        //Initialize file_descriptors
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        flags = fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, flags);
        timeInit(&timeout, SERVER_TIME_OUT, 0);

        maxfdp = sockfd + 1;    //For compatibility issues...
        if ((result = select(maxfdp, &rset, NULL, NULL, &timeout) )< 0)
        {
            perror("select failed");
            return;
        }

        //If the call to select() timed out...
        if (result == 0)
        {
            addr_len = sizeof(their_addr);
            cout << "SERVER TIME OUT!" << endl;
            time_t t = time(0);  // t is an integer type
                cout << "Current Time: " << t << endl;
    
            if(fileOpened)
            {

                //Break if we have finished transmitting
                if(currentAck+1 >= packet_vector.size())
                {
                cout << "hello" << endl;
                    break;
                }
                packetSize = 2000;

                if((numbytes=
                    sendto(sockfd, packet_vector[currentAck],
                           packetSize, 0, (struct sockaddr *)&their_addr, addr_len)) < 0) 
                {
                    perror("Send Failed!");
                    exit(1);
                }
                cout << "Retransmitting packet " << currentAck << endl;
            }
        } 

        if (FD_ISSET(sockfd, &rset)) //If sockfd is ready
        {
    
            temprand1 = (int)rand() % 100;
            //Client's ACK packet LOST!!
            if(temprand1 < Plost)
            {
                numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len);
                cout << "Client's ACK packet LOST!! (" << buf << ") " << endl;

                //Continue to wait to receive a ACK packet from client.
                continue;
            }
            temprand2 = (int)rand() % 100;
            
            //Client's ACK packet CORRUPTED!!
            if(temprand2 < Pcorrupt)
            {    
                cout << "Client's ACK packet CORRUPTED!!" << endl;
                numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len);

                //Continue to wait to receive a ACK packet from client
                continue;
            }

            //=======================//
            //Receive from Client
            //=======================//
            addr_len = sizeof(their_addr);
            numbytes = 1;
            while (numbytes > 0)
            {
                if ((numbytes = 
                    recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len))
                    == -1) 
                {
                    if (errno != EAGAIN)
                    {
                        perror("recvfrom");
                        exit(1);
                    } else
                        break;
                }

                buf[numbytes] = '\0';

                buf_str = buf;

                if (buf_str.find("|", 0, 1) != string::npos)
                    sequenceNum = atoi(buf_str.substr(buf_str.find("|", 0, 1)+1).c_str());
                else
                    sequenceNum = -1;
                cout << "Received Packet from Client: \"" << buf_str << "\"" << endl;
                if (sequenceNum >= 0)
                {
                    currentAck = sequenceNum;
                    cout << "Updating currently ACKed packet counter to " << currentAck << endl;
                }
            }

            //If the sequence number on the packet received is NOT equal to currentPacketToSend+1
            
            //=======================//
            //Extracting file content
            //=======================//
            if(!fileOpened)
            {
                fileOpened = true;
                packet_vector = extractFileToVector(buf);
                if(packet_vector.empty())
                    break;
            }

            //=======================//
            //Sends packet to client
            //=======================//
            
             
            if(currentAck+1 >= packet_vector.size())
            {
            
                break;
            }
               

            packetSize = 2000;
            for (int i = 0; i < cwnd && currentAck+i < packet_vector.size(); i++)
            {
        cout << "sending packet #" << currentAck+i << "(" << i << "/" << cwnd << " in the window)" << endl;
                //Attempt to send packet to client
                if((numbytes=
                    sendto(sockfd, packet_vector[currentAck+i], 
                        packetSize, 0, (struct sockaddr *)&their_addr, addr_len)) < 0) 
                {
                    perror("Send Failed!");
                    exit(1);
                }
            }
        }    
    }
}

int main(int argc, char *argv[])
{
    int const MAXBUFLEN = 2000;
    int myPort;    //User connects to this port
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int cwnd;
    int Plost;
    int Pcorrupt;

    if(argc != 5)
    {
        cerr << "Wrong number of arguments. ( ./sender 4990 5 50 0)" << endl;
        exit(1);    
    }

    myPort = atoi(argv[1]);
    cwnd = atoi(argv[2]);
    Plost = atoi(argv[3]);
    Pcorrupt = atoi(argv[4]);

    memset(&hints, 0, sizeof(hints));
    //Force IPv4
    hints.ai_family = AF_INET;
    //Datagram socket
    hints.ai_socktype = SOCK_DGRAM;
    // use localhost IP
    hints.ai_flags = AI_PASSIVE;

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

    waitToRead(sockfd, buf, MAXBUFLEN, their_addr, cwnd, Plost, Pcorrupt);
    freeaddrinfo(servinfo);

    close(sockfd);
    return 0;
}
