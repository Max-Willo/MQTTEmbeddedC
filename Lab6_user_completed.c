#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <MQTTClient.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define ADDRESS "tcp://localhost:1883"
#define CLIENTID "MYCLIENT"
#define QOS 1
#define TIMEOUT 10000L

#define CHAR_DEV "/dev/Lab6"
#define MSG_SIZE 50

volatile MQTTClient_deliveryToken deliveredtoken;
int master = 0;
int vote, cdev_id;

char add_to_use[13];
char *ext_vote;
int i_ext_vote;	

int myAddr;
MQTTClient client;

char my_message[MSG_SIZE];


void delivered(void *context, MQTTClient_deliveryToken dt){
	printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message){

	int i, dummy;
	char* payloadptr;
	char buffer[MSG_SIZE];
	char tmp[2];
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;

//	printf("Message arrived\n	topic: %d\n	message: ");
	payloadptr = message->payload;


	//WHOIS MESSAGE
	if(strcmp(payloadptr, "WHOIS\n") == 0){
		
		strcpy(buffer, "Max on board ");
		strcat(buffer, add_to_use);
		strcat(buffer, " is the master");
		
		if(master == 1){
			pubmsg.payload = buffer;
			pubmsg.payloadlen = strlen(buffer);
			pubmsg.qos = QOS;
			pubmsg.retained = 0;
			deliveredtoken = 0;
			MQTTClient_publishMessage(client, "EC", &pubmsg, &token);
		}
	}

	//VOTE MESSAGE
	else if(strcmp(payloadptr, "VOTE\n") == 0){

		strcpy(buffer, "# ");
		strcat(buffer, add_to_use);
		vote = rand() & 10 + 1;
		sprintf(tmp, " %d", vote);
		strcat(buffer, tmp);
		printf("\n%s\n", buffer);
		strcpy(my_message, buffer);

		pubmsg.payload = buffer;
		pubmsg.payloadlen = strlen(buffer);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		deliveredtoken = 0;

		MQTTClient_publishMessage(client, "EC", &pubmsg, &token);

		master = 1;

		printf("*******%s\n", my_message);
	}
	
	//# MESSAGE
	else if(strncmp(payloadptr, my_message, 4) == 0){

		int i = atoi(payloadptr + 13);

		if(i != myAddr){
			printf("vote was not my vote\n");
			ext_vote = payloadptr + 16;
			i_ext_vote = atoi(ext_vote);

			if(vote < i_ext_vote) master = 0;
			else if(vote == i_ext_vote){
				ext_vote = payloadptr + 13;
				i_ext_vote = atoi(ext_vote);
				if(myAddr <= i_ext_vote) master = 0;
				else printf("tie won\n");
			}
		}
	}
	
	//NOTE MESSAGE
	else if(payloadptr[0] == '@'){
			
		buffer[0] = payloadptr[1];
		dummy = write(cdev_id, buffer, sizeof(buffer));

		if(master){
			
			pubmsg.payload = buffer;
			pubmsg.payloadlen = strlen(buffer);
			pubmsg.qos = QOS;
			pubmsg.retained = 0;
			deliveredtoken = 0;

			MQTTClient_publishMessage(client, "EC", &pubmsg, &token);

		}
	}

	




	bzero(buffer, MSG_SIZE);

	MQTTClient_freeMessage(&message);

	return 1;
}

void connlost(void *context, char *cause){
	printf("\nConnection lost\n");
	printf("	cause: %s\n", cause);
}

int main(void)
{
	int dummy;
	char buffer[MSG_SIZE];
	struct ifaddrs *ifaddr;
	int family, s;
	char host[NI_MAXHOST];


	// Open the Character Device for writing
	if((cdev_id = open(CHAR_DEV, O_WRONLY)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}


//get own ip

	if(getifaddrs(&ifaddr) == -1) {
		printf("ERROR in getifaddrs\n");
	}

	
	for(struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if(ifa->ifa_addr == NULL) continue;

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET || family == AF_INET6) {
			s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) :
									     sizeof(struct sockaddr_in6),
					host, NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
			if(s != 0){
				printf("ERROR in getnameinfo()\n");
			}
			printf("\t\taddress: <%s>\n", host);
			if(strncmp(host, "128.206.19.", 11) == 0) {strcpy(add_to_use, host); myAddr = atoi(add_to_use + 11);}
		}
	}

//////////////////server stuff
//

	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	int rc;
	char ch[MSG_SIZE];

	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	if((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("Subscribing to topics\n");
	MQTTClient_subscribe(client, "EC", QOS);
	MQTTClient_subscribe(client, "Election", QOS);


	while(1){
		printf("Enter message (! to exit): ");
		fgets(ch, 3, stdin);
		printf("attempting to send %s to the kernel\n", ch);
		if(strcmp(ch, "!") == 0)
			break;

		dummy = write(cdev_id, ch, sizeof(ch));
		if(dummy != sizeof(ch)){ 
			printf("Write failed\n");
			break;
		}
	}

/*

	while(1)
	{
		printf("Enter message (! to exit): ");
		fflush(stdout);
		gets(buffer);	// deprecated function, but it servers the purpose here...
						// One must be careful about message sizes on both sides.
		if(buffer[0] == '!')	// If the first character is '!', get out
			break;
		
		dummy = write(cdev_id, buffer, sizeof(buffer));
		if(dummy != sizeof(buffer)) {
			printf("Write failed, leaving...\n");
			break;
		}
	}
*/
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	
	close(cdev_id);	// close the device.

	return rc;

}









/*
	
*/


