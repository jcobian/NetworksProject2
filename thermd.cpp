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
#include <pthread.h>
#include <stdlib.h>
#include <sstream>
#include "therm.h"

#define DEBUG

using namespace std;

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

//holds data passed from main to thread
typedef struct {
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen;

} thread_info;

void receiveHostFromServer(thread_info* info, Host* host) {

//		n = recvfrom(info->connfd, &hosts[i], sizeof(Host),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen));
	if(recvfrom(info->connfd, &(host->hostName), sizeof(host->hostName),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->numThermometers), sizeof(host->numThermometers),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->sensorNumber), sizeof(host->sensorNumber),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->sensorData), sizeof(host->sensorData),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->lowValue), sizeof(host->lowValue),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->highValue), sizeof(host->highValue),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->timeStamp), sizeof(host->timeStamp),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	if(recvfrom(info->connfd, &(host->action), sizeof(host->action),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen)) < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}


}

//function run by thread upon accepting a new client connection
void *accept_client(void *input) {

	thread_info* info = (thread_info*)input;

	printf("New client accepted!\n");
	printf("\tNew client address:%s\n",inet_ntoa(info->cliaddr.sin_addr));
	printf("\tClient port:%d\n",info->cliaddr.sin_port);

	////////////////////////////////////
	// Receive Server reply
	//

	//Receive the num of sensors 
	int numSensors = 0;
	int n = recvfrom(info->connfd, &numSensors, sizeof(numSensors),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen));
	if (n < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}

	numSensors = ntohl(numSensors);
	printf("\tnum of sensors:%d\n",numSensors);

	Host hosts[numSensors];

	//receive data and store to buffere hosts
	for(int i=0; i<numSensors;i++) {
		//Receive host serialized
		receiveHostFromServer(info, &hosts[i]);
	}

	//Now create the file
	string actionFile = "/var/log/therm/temp_logs/g07_";

	//create file
	string yearandmonth;
	yearandmonth=hosts[0].timeStamp;
	stringstream stream(yearandmonth);
	string year,month;
	stream >> year;
	stream >> month;		
	actionFile+=year;
	actionFile+="_";
	actionFile+=month;
	actionFile+="_";
	actionFile+=hosts[0].hostName;
	const char * file = actionFile.c_str();

	FILE *fpAction = fopen(file,"a");
	int wroteSomething = 0;

	//Now write the data to file
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

		if(hosts[i].action == 0) { //this is a new reading which needs to be stored

			//print timestamp and sensor reading to file
			if(i==0)
				fprintf(fpAction,"%s %.2lf",hosts[i].timeStamp,hosts[i].sensorData);
			else
				fprintf(fpAction," %.2lf",hosts[i].sensorData);
			
			wroteSomething = 1;			
		}
		else if(hosts[i].action == 1) { //the client wants update data back
			int error_msg = 0;
			if(hosts[i].sensorData>hosts[i].highValue) { //if overtemp condition
				error_msg = 1;
			}
			if( sendto(info->connfd, &error_msg, sizeof(error_msg),0,(struct sockaddr*)&(info->cliaddr),(info->clilen))) {
				perror("Error sending message back to client");
				exit(1);
			}
		}
	}
	if(wroteSomething) 
		fprintf(fpAction,"\n");
	wroteSomething = 0;	
	//close file writing
	fclose(fpAction);
	
	printf("File Transfer complete!\n");

	close(info->connfd);

	return 0;
}

int main(int argc, char**argv)
{
	////////////////////////////////////
	// Setup and Connection 
	// 
	int sockfd;
	struct sockaddr_in servaddr;

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

	int val = 1;
	if(	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))==-1) {
		perror("setsockopt failed");
		exit(1);
	}

	//bind to port
	if(	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0) {
		perror("bind failed");
		exit(1);
	}
	if(	listen(sockfd,1024) <0 ) {
		perror("listen failed");
		exit(1);
	}

	////////////////////////////////////
	// Listen to Port
	// 
	
	while(1) {
		thread_info info;
		info.clilen = sizeof(info.cliaddr);
		info.connfd = accept(sockfd,(struct sockaddr *)&info.cliaddr,&info.clilen);

		//spawn a new thread to handle the connection
		pthread_t thread;
		int status = pthread_create(&thread, NULL, accept_client, &info);

		if(status) 
			printf("error creating thread: %i\n", status);
		else
			printf("Thread created\n");
	}
}
