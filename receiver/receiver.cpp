//======================================//
//Client
//======================================//


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
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
    timeout->tv_sec = sec;        //sec
    timeout->tv_usec = milliSec;    //milli-sec
}


void waitToRead(int sockfd, char* buf, int const MAXBUFLEN, struct sockaddr **ai_addr, ofstream &outfile, int Plost, int Pcorrupt)
{
    int maxfdp, result;
    int const SERVER_TIME_OUT = 2;     //Timeout after 3 secondsZ
    int numbytes;
    fd_set rset;
    string sequenceNum = "0";
    string lastPacket;
    string payload;
    char* charPayload;
    size_t ai_addrlen;
    struct timeval timeout;
    string buf_str;
    string ACKMessage;
    size_t tracker1 = 1999;
    size_t tracker2 = 0;
    int temprand1, temprand2;
    size_t nextPacket  = 0;

    while(true)
    {
        //Initialize file_descriptors
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        timeInit(&timeout, SERVER_TIME_OUT, 900);   //999 is HACK TO FIX race condition between server and client

        maxfdp = sockfd + 1;    //For compatibility issues...
        if ((result = select(maxfdp, &rset, NULL, NULL, &timeout) )< 0)
        {
            perror("select failed");
            return;
        }
        if (result == 0)
        {
            
            ACKMessage = "ACK|" ;
            ACKMessage += intToString(nextPacket-1);
        
            cout << "CLIENT TIMEOUT!" << endl;
            time_t t = time(0);  // t is an integer type
            cout << "Current Time: " << t << endl;
            
            cout << "Sending ACK after client timeout: " << ACKMessage << endl;

           if(atoi(lastPacket.c_str()) == 1)
            {
            
            cout << "closing client..." << endl;
                outfile.close();
                break;
            }
            if ((numbytes = sendto(sockfd, ACKMessage.c_str(), strlen(ACKMessage.c_str()), 0, *ai_addr,
                           ai_addrlen)) == -1) 
            {
                perror("Failed to send to ");
                exit(1);
            }
                 
        } 
        
        //If we have received a packet from the sender
        if (FD_ISSET(sockfd, &rset)) 
        {


        //TODO: NEED TO IMPLEMENT FINACK!!!
        
            temprand1 = (int)rand() % 100;

            //Sender packet LOST!!
            if(temprand1 < Plost)
            {
                cout << "Sender packet LOST!!" << endl;
                //if we decide to drop our out going ACK packet, we'll still need to pretend to read from serversock 
                //to get rid of the incoming data
                numbytes = 
                    recvfrom(sockfd, buf, MAXBUFLEN-1, 0, 
                         *ai_addr, 
                         (socklen_t *) &(ai_addrlen) );

                //Continue to wait to receive a packet from sender.
                continue;
            }
            temprand2 = (int)rand() % 100;
            
            //Sender packet CORRUPTED!!
            if(temprand2 < Pcorrupt)
            {    
                cout << "Sender packet CORRUPTED!!" << endl;
                //if we decide to drop our out going ACK packet, we'll still need to pretend to read from serversock 
                //to get rid of the incoming data
                numbytes = 
                    recvfrom(sockfd, buf, MAXBUFLEN-1, 0, 
                         *ai_addr,
                         (socklen_t *) &(ai_addrlen) );

                //Continue to wait to receive a packet from sender.
                continue;
            }

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
            tracker1 = 1999;
            tracker2 = 0;
            sequenceNum = "";
            while(buf[tracker1] != '|')
                tracker1--;
            tracker1--;
            lastPacket = buf[tracker1];
            tracker1--;
            tracker1--;

            while(buf[tracker1] != '|')
            {
                sequenceNum = buf[tracker1] + sequenceNum;
                tracker1--;
            }

            //If the packet sent is the next packet we are waiting for, process it
            if (nextPacket == atoi(sequenceNum.c_str()))
            {
                charPayload = new char[1000];
                while(tracker2 <= tracker1)
                {
                    charPayload[tracker2] = buf[tracker2];
                    tracker2++;
                }
                outfile.write(charPayload, tracker2-1);
                delete[] charPayload;
                cout << "incrementing nextPacket" << endl;
                nextPacket++;
            } else {
                cout << "Incorrect Packet Order received! (" << sequenceNum << " instead of " << nextPacket << ")" << endl;
            }
           
            //=======================//
            //Sends ACK to server
            //=======================//

            ACKMessage = "ACK|" ;
            ACKMessage += intToString(nextPacket-1);
        
            
            cout << "Sending ACK: " << ACKMessage << endl;

            if ((numbytes = sendto(sockfd, ACKMessage.c_str(), strlen(ACKMessage.c_str()), 0, *ai_addr,
                           ai_addrlen)) == -1) 
            {
                perror("Failed to send to ");
                exit(1);
            }
            if(atoi(lastPacket.c_str()) == 1)
            {
            
            cout << "closing client..." << endl;
                outfile.close();
                break;
            }
            
            

        }
    }
}

//TODO: use recvfrom(...) with combination with select(...) to talk back and forth b/t server and client
int main(int argc, char *argv[])
{
    srand(time(0));
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    int rv;
    int numbytes;
    int serverPort;        //clients connect to this server port
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
    
    if ((numbytes = sendto(sockfd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen)) == -1) 
    {
        perror("talker: sendto");
        exit(1);
    }

    outfile.open(argv[3], ios::out | ios::binary);


    //freeaddrinfo(servinfo);

    cout << "talker: sent " << numbytes << " bytes to " << argv[1] << endl;

    cout << "Waiting to receive from server..." << endl;

    //waitToRead(sockfd, buf, MAXBUFLEN, &(p->ai_addr), p->ai_addrlen, outfile);
    waitToRead(sockfd, buf, MAXBUFLEN, &(p->ai_addr), outfile, Plost, Pcorrupt);
    //**


    close(sockfd);
    return 0;
}
