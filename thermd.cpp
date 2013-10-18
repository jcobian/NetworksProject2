/*
 * Jason Wassel, Jonathan Cobian, Charles Jhin
 * 10//13
 * CSE 30264
 * Project 2
 *
 * Server
 */

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <cstdlib>
#include <strings.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <mhash.h>
#include "therm.h"

#define DEBUG

using namespace std;

int main(int argc, char**argv)
{
	////////////////////////////////////
	// Setup and Connection 
	// 
	int sockfd,connfd,n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t clilen;

	//create the socket
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	//build server inet address
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	int s_port = 9768;

	servaddr.sin_port=htons(s_port);

	//bind to port
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(sockfd,1024);

	////////////////////////////////////
	// Listen to Port
	// 
	
	while(1) {
		clilen = sizeof(cliaddr);
		connfd = accept(sockfd,(struct sockaddr *)&cliaddr,&clilen);

		printf("New client accepted!\n");
		printf("\tNew client address:%s\n",inet_ntoa(cliaddr.sin_addr));
		printf("\tClient port:%d\n",cliaddr.sin_port);
		////////////////////////////////////
		// Receive Server reply
		//

		//Receive the num of sensors 
		int numSensors = 0;
		n = recvfrom(connfd, &numSensors, sizeof(numSensors),0,(struct sockaddr*)&cliaddr,&clilen);
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

		numSensors = ntohl(numSensors);
		printf("\tnum of sensors:%d\n",numSensors);

		Host hosts[numSensors];

		for(int i=0; i<numSensors;i++) {
			//Receive the host
			n = recvfrom(connfd, &hosts[i], sizeof(Host),0,(struct sockaddr*)&cliaddr,&clilen);
			if (n < 0) {
				perror("ERROR reading from socket");
				exit(1);
			}
		}

		for(int i=0; i<numSensors; i++) {
			#ifdef DEBUG
			printf("Host %i\n",i);
			printf("\tName: %s\n",hosts[i].hostName);
			printf("\tNum Thermometers: %d\n",hosts[i].numThermometers);
			printf("\tSensor Number: %d\n",hosts[i].sensorNumber);
			printf("\tLow value: %lf\n",hosts[i].lowValue);
			printf("\tHigh value: %lf\n",hosts[i].highValue);
			#endif
            printf("Sensor Data for Host %d is %lf\n",i,hosts[i].sensorData);
		}

		printf("File Transfer complete!\n");
	}
}
