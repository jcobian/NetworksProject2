/* Jonathan Cobian, Chas Jhin, Jason Wassel 
 * CSE 30264 - Computer Networks 
 * Project 2- Thermal Sensors
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "therm.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define DEBUG

//function prototype to read data from sensors
int readSensorData(int sensor, int *result, FILE* fpError);
/* Function to convert Celsius to Fahrenheit*/
float CtoF(float C){return (C*9.0/5.0)+32;}
void writeErrorLog(FILE *fpError,char *message);
int main(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("usage: ./therm <IP address>\n");
		exit(1);
	}
	

	//get host name
	char hostname[32];
	hostname[31] = '\0';
	gethostname(hostname,1023);

	char inputFile[1024] = "/etc/t_client/client.conf";
	char errorLogFile[1024] = "/var/log/therm/error/g07_error_log";
	FILE *fp = fopen(inputFile,"r");
	FILE *fpError = fopen(errorLogFile,"a");
	if(fpError==NULL)
	{
		printf("Error trying to open error file\n");
		exit(1);
	}
	if(fp==NULL)
	{
		char *message;
		sprintf(message,"Cannot open config file %s",inputFile);
		writeErrorLog(fpError,message);
		exit(1);
	}
	char buffer[1024];
	int numSensors;

	//read first line of file, which holds number of sensors
	if(fgets(buffer,sizeof(buffer),fp))
	{
		numSensors = atoi(buffer);
	}

	//create array of stuct Hosts
	Host hosts[numSensors]; 
	int i = 0;
	//for each subsequent line, add to array of structs
	while(fgets(buffer,sizeof(buffer),fp))
	{
		//add name of the host
		strcpy(hosts[i].hostName,hostname);
		//add number of thermometers to struct
		hosts[i].numThermometers = numSensors;

		//add sensor number (/dev/gotemp = 0 and /dev/gotemp2 = 1)
		//first line would be 0 for first sensor and second line is for 2nd sensor
		hosts[i].sensorNumber = i;

		double low, high;
		//scan for low and high values
		int n = sscanf(buffer,"%lf %lf",&low,&high);
		//if succesfully read two items
		if(n==2)
		{
			//store low and high values
			hosts[i].lowValue = low;
			hosts[i].highValue = high;
		}
		else
		{
			#ifdef DEBUG
			printf("Error reading %s\n",inputFile);
			#endif
			char *message;
			sprintf(message,"Error reading file %s",inputFile);
			writeErrorLog(fpError,message);
			exit(1);
		}
		#ifdef DEBUG
		printf("Host %i\n",i);
		printf("\tName: %s\n",hosts[i].hostName);
		printf("\tNum Thermometers: %d\n",hosts[i].numThermometers);
		printf("\tSensor Number: %d\n",hosts[i].sensorNumber);
		printf("\tLow value: %lf\n",hosts[i].lowValue);
		printf("\tHigh value: %lf\n",hosts[i].highValue);
		#endif
		i++;
	}
	time_t rawtime;
	struct tm *tm;
	//load sensor data into structs
	for(i=0;i<numSensors;i++)
	{
		memset((char *)&tm,0,sizeof(tm));
		memset((char *)&rawtime,0,sizeof(rawtime));
			
		int sensData;
		if(readSensorData(i,&sensData,fpError)==0)
		{
			//put timestamp in struct
			time(&rawtime);
			tm = localtime(&rawtime);
			strftime(hosts[i].timeStamp,32,"%Y %m %d %H %M",tm);
			//put sensor data into struct
			hosts[i].sensorData = sensData;
			
			#ifdef DEBUG
			printf("Sensor Data for Host %d is %lf\n",i,hosts[i].sensorData);
			printf("Timestamp for Host %d is %s\n",i,hosts[i].timeStamp);
			#endif
		}
		else
		{
			//error logs written in read sensor data function
			exit(1);
		}
		
	}

	int sockfd, n;
	struct sockaddr_in servaddr;
	
	//ip address where server is
	char* ap_addr = argv[1];
	//char *ap_addr = "123.4.5.6";
	char* port = "9768";

	//create the socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0) {
	writeErrorLog(fpError,"Error opening socket");
	perror("ERROR opening socket");
	exit(1);
    }


	//build server inet address
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(ap_addr); //the address
    servaddr.sin_port=htons(atoi(port)); //the port

	//connect
    if(connect(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
	writeErrorLog(fpError,"Error connecting");
        perror("ERROR connecting");
	exit(1);
    }
	
	int send_numSensors = htonl(numSensors);
	
	if( sendto(sockfd,&send_numSensors,sizeof(send_numSensors),0,
		(struct sockaddr *) &servaddr,sizeof(servaddr)) < 0) {
		writeErrorLog(fpError,"Error writing number of sensors socket");
		perror("ERROR writing to socket");
		exit(1);
	}


	//send packet of hosts to server
	for(i=0;i<numSensors;i++)
	{
		//set action to 0 to send
		hosts[i].action = 0;
		//here write to server
		
		if( sendto(sockfd,&hosts[i],sizeof(hosts[i]),0,
			(struct sockaddr *) &servaddr,sizeof(servaddr)) < 0) {
			writeErrorLog(fpError,"Error writing struct socket");
			perror("ERROR writing to socket");
			exit(1);
		}
		
    }	
	
	close(sockfd);
	fclose(fp);
	fclose(fpError);
	return 0;
} //end main

/*Code taken from Jeff Sadowski <jeff.sadowski@gmail.com> Under the terms of the 
* GPL http://www.gnu.org/copyleft/gpl.html
Code slighly modified by authors of this program (Cobian, Jhin, Wassel)
//reads data from gotemp if sensor=0, from gotemp2 if sensor = 1. stores reading into result. 
//returns 0 if succesfull, 1 if error occured
*/







int readSensorData(int sensor, int *result, FILE* fpError)
{
	char *fileName="/dev/gotemp";
	char *fileName2="/dev/gotemp2";
	struct stat buf, buf2;
	struct packet temp, temp2;

	/* I got this number from the GoIO_SDK and it matched 
	   what David L. Vernier got from his Engineer */

	float conversion=0.0078125;
	int fd, fd2;
	if(sensor==0)
	{
		if(stat( fileName, &buf ))
		{
		   if(mknod(fileName,S_IFCHR|S_IRUSR|S_IWUSR|S_IRGRP |S_IWGRP|S_IROTH|S_IWOTH,makedev(180,176)))
		   {
			char *message;
			sprintf(message,"Cannot create device %s need to be root",fileName);
			writeErrorLog(fpError,message);
			  return 1; 
		   }
		}
	}
	if(sensor==1)
	{
		if(stat( fileName2, &buf2 ))
		{
		   if(mknod(fileName2,S_IFCHR|S_IRUSR|S_IWUSR|S_IRGRP |S_IWGRP|S_IROTH|S_IWOTH,makedev(180,177)))
		   {
			char *message;
			sprintf(message,"Cannot create device %s need to be root",fileName2);
			writeErrorLog(fpError,message);
			  return 1; 
		   }
		}
	}
	/* If cannot open, check permissions on dev, and see if it is plugged in */
	if(sensor==0)
	{
		if((fd=open(fileName,O_RDONLY))==-1)
		{
			char *message;
			sprintf(message,"Could not read %s",fileName);
			writeErrorLog(fpError,message);
			return 1; 
		}
	}
	if(sensor==1)
	{
		if((fd2=open(fileName2,O_RDONLY))==-1)
		{
			char *message;
			sprintf(message,"Could not read %s",fileName2);
			writeErrorLog(fpError,message);
			return 1; 
		}
	}
	/* if cannot read, check is it plugged in */
	if(sensor==0)
	{
		if(read(fd,&temp,sizeof(temp))!=8)
		{
			char *message;
			sprintf(message,"Error reading %s",fileName);
			writeErrorLog(fpError,message);
			return 1; 
		}
	}
	if(sensor==1)
	{
		if(read(fd2,&temp2,sizeof(temp))!=8)
		{
			char *message;
			sprintf(message,"Error reading %s",fileName2);
			writeErrorLog(fpError,message);
			return 1; 
		}
	}
	if(sensor==0)
	close(fd);
	if(sensor==1)
	close(fd2);


	if(sensor==0)
	*result = (CtoF(((float)temp.measurement0)*conversion));

	if(sensor==1)
	*result = (CtoF(((float)temp2.measurement0)*conversion));
	
	
	return 0;

}
void writeErrorLog(FILE *fpError,char *message)
{
	char errorLogDate[32];
	time_t rawtime;
	struct tm *tm;	
	time(&rawtime);
	tm = localtime(&rawtime);
	strftime(errorLogDate,32,"%Y %m %d %H %M",tm);
	fprintf(fpError,"%s %s\n",errorLogDate,message);

}
