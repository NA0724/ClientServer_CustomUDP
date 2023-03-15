// Server side implementation
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

//DATA Packet format
//data pkt format
struct dataPacket{
    uint16_t start_ID;
    uint8_t  client_ID;
    uint16_t data;
    uint8_t  segment_Number;
    uint8_t  length;
    char payload[255];
    uint16_t end_ID;
};

//ACK Packet format
struct ackPacket{
    uint16_t start_ID;
    uint8_t  client_ID;
    uint16_t ack;
    uint8_t  segment_Nounmber;
    uint16_t end_ID;
};

//REJECT packet format
struct rejectPacket{
    uint16_t start_ID;
    uint8_t client_ID;
    uint16_t reject;
    uint16_t reject_Sub_Code;
    uint8_t  received_Segment_Number;
    uint16_t end_ID;
};

//print  the packet
void showPacket(struct dataPacket data) {
    printf("\n-------------------------------------------------------");
    printf("\n INFO: Received packet details:\n");
	printf("  Payload: %s\n",data.payload);
}
//create reject packet and assign values
struct rejectPacket createPKTforREJECT(struct dataPacket data)
{
    struct rejectPacket reject;
    
    reject.start_ID = data.start_ID;
    reject.client_ID = data.client_ID;
    reject.received_Segment_Number = data.segment_Number;
    reject.reject = REJECT;
    reject.end_ID = data.end_ID;
    return reject;
}
//create ack packet and assign values
struct ackPacket createPKTforACK(struct dataPacket data)
{
    struct ackPacket ack;
    
    ack.start_ID = data.start_ID;
    ack.client_ID  = data.client_ID;
    ack.ack       = ACK;
    ack.segment_Nounmber     = data.segment_Number;
    ack.end_ID = data.end_ID;
    return ack;
}

int main() {
	int socket_fd;
	struct sockaddr_in server_Addr;
    struct sockaddr_storage storage;
    socklen_t address_Size;
    struct dataPacket data_packet;
    struct ackPacket ack_packet;
    struct rejectPacket reject_packet;
    int packet_Seq = 1;
	int n;
    
    //store all packets in buffer
    int buffer[20];
    for(int j = 0; j < 20;j++) {
        buffer[j] = 0;
    }
	if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&server_Addr, 0, sizeof(server_Addr));
	
	// Server information
	server_Addr.sin_family = AF_INET; // IPv4
	server_Addr.sin_addr.s_addr = INADDR_ANY;
	server_Addr.sin_port = htons(PORT);
	
	// Bind the socket with the server address
	if ( bind(socket_fd, (const struct sockaddr *)&server_Addr,
			sizeof(server_Addr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

    address_Size = sizeof(server_Addr);
    printf("\n Server started successfully \n");
    while(1)
    {
	    n = recvfrom(socket_fd,&data_packet, sizeof(struct dataPacket),
				0, ( struct sockaddr *) &storage,
				&address_Size);
        showPacket(data_packet);
        
        buffer[data_packet.segment_Number]++;
        if(data_packet.segment_Number == 11 || data_packet.segment_Number == 12)
        {
            buffer[data_packet.segment_Number] = 1;
        }
        int length = strlen(data_packet.payload);
        
	
        if(buffer[data_packet.segment_Number] != 1){
			reject_packet = createPKTforREJECT(data_packet);
            reject_packet.reject_Sub_Code = REJECT_DUPLICATE_PACKET;
			sendto(socket_fd, &reject_packet, sizeof(struct rejectPacket),
                   0, (const struct sockaddr *) &storage,
                   address_Size);
            printf("\n ERROR : Packet error Duplicate Packet");
        }
        else if(length != data_packet.length){
			reject_packet = createPKTforREJECT(data_packet);
            reject_packet.reject_Sub_Code = REJECT_LENGTH_MISMATCH;
			sendto(socket_fd, &reject_packet, sizeof(struct rejectPacket),
                   0, (const struct sockaddr *) &storage,
                   address_Size);
            printf("\n ERROR : Packet error length mismatch");
        }
        else if(data_packet.end_ID != END_OF_PACKET_ID){
			reject_packet = createPKTforREJECT(data_packet);
            reject_packet.reject_Sub_Code = REJECT_END_OF_PACKET_MISSING;
			sendto(socket_fd, &reject_packet, sizeof(struct rejectPacket),
                   0, (const struct sockaddr *) &storage,
                   address_Size);
            printf("\n ERROR : Packet error End of Packet missing");
        }
        else if(data_packet.segment_Number != packet_Seq &&
                data_packet.segment_Number != 11 && data_packet.segment_Number != 12){
					reject_packet = createPKTforREJECT(data_packet);
            		reject_packet.reject_Sub_Code = REJECT_OUT_OF_SEQUENCE;
					sendto(socket_fd, &reject_packet, sizeof(struct rejectPacket),
                   		0, (const struct sockaddr *) &storage,
                   			address_Size);
            		printf("\n ERROR : Packet error Out of Sequence");
        }
   		else{
			if(data_packet.segment_Number == 11) {
				sleep(10); // make the client wait and get no response from server       
			}
    		ack_packet = createPKTforACK(data_packet);
			sendto(socket_fd, &ack_packet, sizeof(struct ackPacket),
				0, (const struct sockaddr *) &storage,
					address_Size);
			printf("\n RESPONSE: ACK recieved \n");
   		}
        packet_Seq++;
    }   
}