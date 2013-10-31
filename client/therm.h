typedef struct {
	char hostName[32];
	int numThermometers; //from config file
	int sensorNumber; // /dev/gotemp==sensor 0, /dev/gotemp2 == sensor 1
	double sensorData;
	double lowValue; //from config
	double highValue; //from config
	char timeStamp[32]; 
	int action; //send = 0, request = 1

}Host;

struct packet {
	unsigned char measurements;
	unsigned char counter;
	int16_t measurement0;
	int16_t measurement1;
	int16_t measurement2; 
	};

