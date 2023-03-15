// Client side implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 40532
#define MAXLINE 1024
#define ACK_TIMEOUT 3						// ack_timer

#define START_OF_PACKET_ID 0XFFFF 			// Start of Packet identifie
#define END_OF_PACKET_ID 0XFFFF 			// End of Packet identifie
#define CLIENT_ID_MAX 0XFF					// Client Id (Maximum) 
#define MAX_LENGTH 0XFF						// Length (Maximum )

#define DATA 0XFFF1							// Data Packet Type
#define ACK 0XFFF2							// Acknowledge Packet
#define REJECT 0XFFF3						// Reject Packet

#define REJECT_OUT_OF_SEQUENCE 0XFFF4		// REJECT out of sequence
#define REJECT_LENGTH_MISMATCH 0XFFF5		// REJECT length mismatch
#define REJECT_END_OF_PACKET_MISSING 0XFFF6 // REJECT End of packet missing
#define REJECT_DUPLICATE_PACKET 0XFFF7		// REJECT Duplicate packet

//DATA packet format
struct dataPacket{
    uint16_t start_ID;
    uint8_t client_ID;
    uint16_t data;
    uint8_t segment_Number;
    uint8_t length;
    char payload[255];
    uint16_t end_ID;
};

//REJECT packet format
struct rejectPacket{
    uint16_t start_ID;
    uint8_t client_ID;
    uint16_t reject;
    uint16_t reject_Sub_Code;
    uint8_t received_Segment_Number;
    uint16_t end_ID;
};

//fill data in dataPacket to create the data packet
struct dataPacket fillDataPacket(){
    struct dataPacket data;
    data.start_ID = START_OF_PACKET_ID;
    data.client_ID = CLIENT_ID_MAX;
    data.data = DATA;
    data.end_ID = END_OF_PACKET_ID;
    
    return data;
}
//print  the packet
void showPacket(struct dataPacket data) {
    
    printf("\n------Received packet information------ \n");
    printf("  Segment number : %d\n",data.segment_Number);
    printf("  Length %d\n",data.length);
	printf("  Payload: %s\n",data.payload);
}

int main() {
	int socket_fd;
	char buffer[MAXLINE];
	struct sockaddr_in client_Addr;
    struct dataPacket data_packet;
    struct rejectPacket reject_packet;
    socklen_t addr_size;
    FILE *fp;
    char line[255];
    int segNum = 1;
    int retryCounter = 0;
	int n, len;
	struct timeval timer;

	if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&client_Addr, 0, sizeof(client_Addr));
    client_Addr.sin_family = AF_INET;
    client_Addr.sin_port = htons(PORT);
    client_Addr.sin_addr.s_addr = INADDR_ANY;

    timer.tv_sec = ACK_TIMEOUT;						//Socket timeout
    timer.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timer,sizeof(struct timeval));
    addr_size = sizeof(client_Addr);
    data_packet = fillDataPacket();
    fp = fopen("packet_payload.txt","r");			//get contents of payload from txt file
    if(fp == NULL){
        printf("ERROR: No such file found\n");
    }

    while(fgets(line,sizeof(line),fp) != NULL)
    {
        n = 0;
        data_packet.segment_Number = segNum;
        strcpy(data_packet.payload,line);
        data_packet.length = strlen(data_packet.payload);
        retryCounter = 0;
        if(segNum == 10){							// out of seq error packet
            data_packet.segment_Number = data_packet.segment_Number + 4;
        }
        if(segNum == 8){							//length mismatch packet
            data_packet.length++;
        }
        if(segNum == 7){  							// duplicate packet
            data_packet.segment_Number = 3;
        }
		if(segNum == 9){  							// end of packet error
            data_packet.end_ID = 0;
        }
        if(segNum != 9){
            data_packet.end_ID = END_OF_PACKET_ID;
        }
        
        printf("Sending data......\n");
        showPacket(data_packet);
        while(n <= 0 && retryCounter < 3)
        {
	      sendto(socket_fd, &data_packet, sizeof(struct dataPacket),
		            0, (const struct sockaddr *) &client_Addr,
                 addr_size);

		
	       n = recvfrom(socket_fd,&reject_packet, sizeof(struct rejectPacket),
				0, (struct sockaddr *) &client_Addr,
				&addr_size);
    
	        printf(" Response from Server : "); //check reject pkt struct 
            if(n <= 0){
                printf("ERROR  Server does not respond");
                printf("\n Sending Packet Again.............. \n");
                retryCounter++;
            }
            else if(reject_packet.reject == ACK){
                printf("ACK packet received successfully!");
            }
            else if(reject_packet.reject == REJECT){
                printf("REJECT received");
				if(reject_packet.reject_Sub_Code == REJECT_LENGTH_MISMATCH){
					printf("\n Reject Type: Length Mismatch");
				}else if(reject_packet.reject_Sub_Code == REJECT_END_OF_PACKET_MISSING){
					printf("\n Reject Type: End of Packet Mismatch");
				}else if(reject_packet.reject_Sub_Code == REJECT_OUT_OF_SEQUENCE){
					printf("\n Reject Type: Out of Sequence");
				}else if(reject_packet.reject_Sub_Code == REJECT_DUPLICATE_PACKET){
					printf("\n Reject Type: Duplicate Packet");
				}                
            }           
        }
        if(retryCounter >= 3){
            printf("\n ERROR : Server does not respond");
            exit(0);
        }
        segNum++;
        printf("\n-----------------------------------------------------------------------------------------\n");
    }
    fclose(fp);
}

