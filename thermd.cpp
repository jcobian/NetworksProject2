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

//function run by thread upon accepting a new client connection
void *accept_client(void *input) {

	thread_info* info = input;
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

	for(int i=0; i<numSensors;i++) {
		//Receive the host
		n = recvfrom(info->connfd, &hosts[i], sizeof(Host),0,(struct sockaddr*)&(info->cliaddr),&(info->clilen));
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

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
		/*				
		//read the file here to get the last read sensor data
		FILE *fp = fopen(actionFile,"r");	
		char buffer[1024];
		double sensData1=0,sensData2=0;
		if(fp!=NULL)
		{
		while(fgets(buffer,sizeof(buffer),fp))
		{
			int n = sscanf("%d %d %d %d %d %lf %lf",NULL,NULL,NULL,NULL,NULL,&sensData1,&sensData2);
				
		}
		}
		fclose(fp);*/

		//open or create file
		FILE *fpAction = fopen(file,"a");
		int wroteSomething = 0;
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
				{
					fprintf(fpAction," %.2lf",hosts[i].sensorData);
				
				}
				wroteSomething = 1;			
		
			}
		}
		if(wroteSomething) 
			fprintf(fpAction,"\n");
		wroteSomething = 0;	
		//close file writing
		fclose(fpAction);
	}
	
	printf("File Transfer complete!\n");

	return 0;
}

int main(int argc, char**argv)
{
	////////////////////////////////////
	// Setup and Connection 
	// 
	int sockfd,connfd,n;
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

	//bind to port
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(sockfd,1024);

	////////////////////////////////////
	// Listen to Port
	// 
	
	while(1) {
		thread_info info;
		info.clilen = sizeof(info.cliaddr);
		info.connfd = accept(sockfd,(struct sockaddr *)&info.cliaddr,&info.clilen);

		//spawn a new thread to handle the connection
		int status = pthread_create(NULL, NULL, accept_client, &info);

		if(status) 
			printf("error creating thread: %i\n", status);
		else
			printf("Thread created\n");
	}
}
