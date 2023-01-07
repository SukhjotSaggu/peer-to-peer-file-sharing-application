
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>


#define BUFSIZE 100
#define ACK_PDU_TYPE 'A'
#define ERROR_PDU_TYPE 'E'

// General PDU structure
struct pdu {
char type;
char data[101];
} NamePDU, IndexPDU, SendPDU,TCPPDU;

// PDU Structure for Registering Content
struct RPDU {
	char type;
	char pName[10];
	char cName[10];
	char address[80];
};

// PDU Structure for Deregistering Content
struct DRPDU {
	char type;
	char pName[10];
	char cName[10];
};

int main (int argc, char** argv) {

	int s, new_s, port;
	struct	sockaddr_in server;
	char *host;
	struct hostent *phe;
	int TCPs,alen, new_sd, client_len;
	struct	sockaddr_in TCPserver, client;
	struct hostent *TCPphe;
	int n;
	char Tport [80], Thost [80], userName[10], ER [10];;

	// clear the character arrays (initiated with random values)
	memset(userName, 0 , sizeof(userName));
	memset(ER, 0 , sizeof(ER));
	memset(Tport, 0 , sizeof(Tport));
	memset(Thost, 0 , sizeof(Thost));

    //store IP address and Port Number from the terminal
    switch(argc) {
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    memset(&server, 0 , sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if  (phe = gethostbyname(host)) {
        memcpy(&server.sin_addr, phe->h_addr, phe->h_length);
    } else if ((server.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        fprintf(stderr, "Cant get host entry \n");
    }

    // Create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Can't create socket\n");
        exit(1);
    }

    // Connect the UDP socket
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        fprintf(stderr, "Can't connect to %s \n", host);


    //Setting up the TCP connection
    memset(&TCPserver, 0 , sizeof(TCPserver));
    TCPserver.sin_family = AF_INET;
    TCPserver.sin_port = htons(0);
    TCPserver.sin_addr.s_addr=htonl(INADDR_ANY);

    // Create a TCP socket
    if ((TCPs = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "TCP:Can't create socket\n");
    }

	// bind the TCP Socket
    if (bind(TCPs,(struct sockaddr *)&TCPserver, sizeof(TCPserver)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	alen=sizeof(struct sockaddr_in);
	getsockname(TCPs,(struct sockaddr *)&TCPserver, &alen);

	// listen for up to 5 queued TCP connections
	listen(TCPs, 5);

	// create file descriptor sets (socket descriptors count as file descriptors)
	fd_set afds, bfds;

	//Read the username provided by the peer
	printf("Please enter your user name:\n");
    memset(userName, 0, sizeof(userName));
    n=read(0, userName, sizeof(userName));
    userName[n-1]='\0';
	
	//store the TCP host and port values in Thost and Tport
   	sprintf(Thost, "%d",TCPserver.sin_addr.s_addr);
   	printf("\n%s's TCP port number: %d\n", userName,TCPserver.sin_port);
   	sprintf(Tport, "%d",TCPserver.sin_port);
	displayInfo();

	while(1){
		//clear the afds file descriptor set
		FD_ZERO(&afds);

		// add TCP socket descriptor to the file descriptor set  (set corresponding afds bit to 1)
		FD_SET(TCPs, &afds);

		// adds 0 to the file descriptor set --> represents a pending TCP connection when a peer wants to do something
		FD_SET(0, &afds);
		memcpy(&bfds, &afds, sizeof(bfds));

		// if there is data available to use in one of the socket descriptors in the file descriptor set, than select will return.
		// if not, then select will hold here until a tcp connection is correctly established or something is returned from stdin
		select(FD_SETSIZE, &bfds,NULL,NULL,NULL);

		//cehcking if data is ready to be used at the TCP socket (checking if the set is set or not)
		// if its ready for use --> a download from one peer to another will occur
		if(FD_ISSET(TCPs,&bfds)){
			client_len = sizeof(client);
	  		new_sd = accept(TCPs, (struct sockaddr *)&client, &client_len);
	  		if(new_sd < 0){
			    fprintf(stderr, "Error accepting TCP Connection \n");
			}
			else{
				dataTransfer(new_sd);
			}

		}
	 	//check if there is a pending TCP connection -> peer is inputting values from the terminal
		if(FD_ISSET(0,&bfds)){
			interface(userName,s,Tport,Thost);
		}
	}
}

void displayInfo() {
	printf("\nR - Register Content\nT - Content DeRegistration\nD - Content Download Request\nO - List of Online Registered Content\nS - Search for Content\nQ - Quit\n\n");
}

char * convertRpdutoString(struct RPDU pdu, char * stringRPDU){ //convert RPDU to string for transmission
    bzero(stringRPDU, sizeof(101));
    sprintf(stringRPDU, "%c", pdu.type);
    strcat(stringRPDU, ",");
    strcat(stringRPDU, pdu.pName);
    strcat(stringRPDU, ",");
    strcat(stringRPDU, pdu.cName);
    strcat(stringRPDU, ",");
    strcat(stringRPDU, pdu.address);

    return stringRPDU;
};

char * convertDRpdutoString(struct DRPDU pdu, char * stringDRPDU){ //convert DRPDU to string for transmission
    bzero(stringDRPDU, sizeof(21));
    sprintf(stringDRPDU, "%c", pdu.type);
    strcat(stringDRPDU, ",");
    strcat(stringDRPDU, pdu.pName);
    strcat(stringDRPDU, ",");
    strcat(stringDRPDU, pdu.cName);

    return stringDRPDU;
};
void dataTransfer(int new_sd){
	char BUFF[100], message[101], con[101];
	memset(BUFF, 0, strlen(BUFF));
	memset(message, 0, strlen(message));
	memset(con, 0, strlen(con));
	memset(TCPPDU.data, 0, strlen(TCPPDU.data));
	FILE*fp;

	read(new_sd, BUFF,100);
	TCPPDU.type = BUFF[0];
	int  i, j;
	for (i = 0; i < 100; i++) {
		TCPPDU.data[i] = BUFF[i + 1];
	}
	if(TCPPDU.type == 'D'){
		//Tries to find file with the name provided in the PDU
		fp =fopen(TCPPDU.data, "r");
		if(fp==NULL){
			printf("There was an error finding the file!\n");
			strcat(message, "E");
			strcat(message, "Error: file not found!\n");
		}
		else{
			while ((j = (fread(con, 1, 100, fp)))!=0){ 
				//printf("\nCon: %s\nNum of bytes: %d\n", con, j);
				strcat(message, "C");
				strcat(message, con);
				//printf("\nMessage: %s\n", message);
				write(new_sd, message, j+1);
				memset(message, '\0', strlen(message));
				memset(con, '\0', strlen(con));
			}
		}
		//write(new_sd, message, 100);
		fclose(fp);
	}
	else{
		printf("There was an error receiving TCP socket from peer\n");
	}
	close(new_sd);
}

void interface(char* userName, int s, char* Tport, char* Thost){

    char  type, data[101], content[1000], buffer[101];
    int n,TCPport;
    FILE*fp;
    struct RPDU rpdu;
    struct DRPDU drpdu;
    char ER [10];
    char pName[10], cName[10], Tadd[80] ;
    char*regPDU, *deRegPDU;
    regPDU=malloc(101);
    deRegPDU=malloc(101);
    char SinChar;
    int 	STCP;
	struct	sockaddr_in serverTCP;
	struct hostent *phe;

    //get a single character from stdin
    type=getchar();
    switch(type){
    	case 'D':
			// DOWNLOADING CONTENT
			printf("Enter the name of the Content: \n");
			memset(NamePDU.data, 0, sizeof(NamePDU.data));
			n=read(0, NamePDU.data, BUFSIZE);
			NamePDU.type= 'S';
			NamePDU.data[n-1]='\0';
			
			//send the file name to Index
			if( write(s, &NamePDU, n+1) <0){
				printf("\nError sending file name to Index\n");
				displayInfo();
					break;
			}
			
			//get the response from Index
			memset(data, 0, sizeof(data));
			n=read(s, data, BUFSIZE);
			IndexPDU.type = data[0];
			memset(IndexPDU.data, 0, sizeof(IndexPDU.data));
			int i;
			for(i=0; i<n; i++){
				IndexPDU.data[i] = data[i+1];
			}
			IndexPDU.data[n-1] ='\0';
			TCPport = atoi(IndexPDU.data);
   			//check if there was an error
			if(IndexPDU.type == 'E'){
				printf("Error: Requested Content Does Not Exist\n");
				displayInfo();
				break;
			}
			//Set up TCP connection with the Peer
			//empty out the Server structure and set default Values
			memset(&serverTCP, 0 , sizeof(serverTCP));
			serverTCP.sin_family = AF_INET;
			serverTCP.sin_port = TCPport;

			// Make sure everything good with IP and copy into server
			if  (phe = gethostbyname(Thost)) {
				memcpy(&serverTCP.sin_addr, phe->h_addr, phe->h_length);
			} else if ((serverTCP.sin_addr.s_addr = inet_addr(Thost)) == INADDR_NONE) {
				fprintf(stderr, "TCP:Cant get host entry \n");
			}

    		// Create a stream socket
			if ((STCP= socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				fprintf(stderr, "TCP:Can't create socket\n");
				// exit(1);
			}

    		printf("Connected to Port: %d\n", serverTCP.sin_port );
    		// Connect the socket
			if (connect(STCP, (struct sockaddr *)&serverTCP, sizeof(serverTCP)) < 0)
				fprintf(stderr, "TCP:Can't connect to %s \n", Thost);

			//sending the file name to the peer that has the content
			NamePDU.type='D';
			write(STCP, &NamePDU, sizeof(NamePDU.data) + 1);
			//recieving the reponse from the peer
			memset(data, 0, sizeof(data));
			bzero(content,1000);
			SendPDU.type = '\0';
			//while( n=read(STCP, data, BUFSIZE) ){
			while( n=read(STCP, data, 101) ){
				if (SendPDU.type == '\0')
					SendPDU.type= data[0];
				int i;
				//printf("\nData from read: %s\n", data);
				for(i=0; i<n; i++){
					SendPDU.data[i] = data[i+1];
				}
				//SendPDU.data[n-1]='\0';
				//printf("\nData: %s\n", SendPDU.data);
				strcat(content, SendPDU.data);
				memset(SendPDU.data, 0, sizeof(SendPDU.data));
				//printf("\nContent: %s\n", content);
				memset(data, 0, sizeof(data));
			}

			//check if there was an error with the download
			if(SendPDU.type == 'E'){
				printf("Error downloading content from peer");
				displayInfo();
				break;
			}
			//if no errors, store the downloaded content in a new file
			else{
				fp=fopen(NamePDU.data,"w");
				fputs( content,fp);
				fclose(fp);
				memset(content, 0, sizeof(content));
				printf("%s DOWNLOADED!",NamePDU.data);

			}
			close(STCP);

     		rpdu.type = 'R';
    		//save peer name in rpdu structure
			strcpy(rpdu.pName,userName);
			strcpy(rpdu.cName,NamePDU.data);
			sprintf(Tadd, "%d",serverTCP.sin_port);
			strcpy(rpdu.address,Tport);

			//convert rpdu to string and send it to the index server
			regPDU = convertRpdutoString(rpdu, regPDU);

			write(s,regPDU,101);
    		//get response from index
    		int receive=0;
       		receive = read(s, buffer,101);
			//if sucessfully registered, index server returns an Ack PDU, so we let the user know and clear all variables
			if(buffer[0]==ACK_PDU_TYPE){
					printf("\nContent registered successfully!\n");
					bzero(rpdu.cName,10);
					bzero(buffer,101);
			}
    		//if invalid peer name, index server returns an error PDU
			else if (buffer[0]==ERROR_PDU_TYPE){
				printf("\nError Registering At Index,Change Peer Name!\n");
				bzero(rpdu.cName,10);
				bzero(buffer,101);
			}

			displayInfo();
			break;
  		case 'S':
			// SEARCHING FOR CONTENT
    		// Get the name of the content the peer is looking for
			printf("Enter the name of the Content: \n");
			memset(NamePDU.data, 0, sizeof(NamePDU.data));
			n=read(0, NamePDU.data, BUFSIZE);
			NamePDU.type= 'S';
			NamePDU.data[n-1]='\0';
    		// Send the content name to the index server
			if( write(s, &NamePDU, n+1) <0){
				printf("\nAn error occured when sending the content name to the Index server\n");
				displayInfo();
				break;
			}
    		
			// Read the response from the index server and load it into the IndexPDU Struct 
			memset(data, 0, sizeof(data));
			n=read(s, data, BUFSIZE);
			IndexPDU.type = data[0];
			memset(IndexPDU.data, 0, sizeof(IndexPDU.data));
			for(i=0; i<n; i++){
				IndexPDU.data[i] = data[i+1];
			}
			IndexPDU.data[n-1] ='\0';
			TCPport = atoi(IndexPDU.data);
    		//check if there was an error
			if(IndexPDU.type == 'E'){
				printf("Error Content Doesn't Exist\n");
				displayInfo();
				break;
			}
   			//If the server has the content
			else if( IndexPDU.type = 'S'){
				printf("\nContent Can Be Found At Port: %d\n",TCPport);
			}
			displayInfo();
			break;
   		case 'O':

			SinChar='O';
			//Send the PDU type 'O' to the server
			if( write(s,&SinChar, 1) <0){
				printf("\nError sending the 'O' type request to the index server\n");
				displayInfo();
				break;
			}
    		//get the response from Index
			memset(data, 0, sizeof(data));
			n=read(s, data, BUFSIZE);
			IndexPDU.type = data[0];
			memset(IndexPDU.data, 0, sizeof(IndexPDU.data));
			for(i=0; i<n; i++){
				IndexPDU.data[i] = data[i+1];
			}
			IndexPDU.data[n-1] ='\0';
    		//check if there was an error
			if(IndexPDU.type == 'E'){
				printf("\nError returned from the index server\n");
				displayInfo();
				break;
			}
    		//If the server has the content
			else if( IndexPDU.type = 'O'){
				printf("Registered Content:\n%s\n",IndexPDU.data);
			}
    		displayInfo();
    		break;
    	case 'R':
			rpdu.type = 'R';
			fflush(stdout);
			fgets(ER,10, stdin);

    		//get the name of the content to be registered
			printf("Enter Content Name: \n");
			fflush(stdout);
			fgets(cName,10, stdin);
			strtok(cName,"\n");
   			
			//set up the registration PDU
			strcpy(rpdu.pName,userName);
			strcpy(rpdu.cName,cName);
			strcpy(rpdu.address,Tport);

    		//convert the registration PDU to a string and send to index server
    		regPDU = convertRpdutoString(rpdu, regPDU);
    		write(s,regPDU,101);

    		//recevie response from index server
        	receive = read(s, buffer,101);
			
			//check is ack PDU was sent back (meaning that registration was successful)
			if(buffer[0]==ACK_PDU_TYPE){
					printf("Content Registered Successfully!\n");
					bzero(rpdu.cName,10);
					bzero(buffer,101);
			}
			//if ack is not returned, its because a peer is trying to register the a piece of content that it already has registered
			else if (buffer[0]==ERROR_PDU_TYPE){
				printf("\nError Registering At Index,Change Peer Name!\n");
				bzero(rpdu.cName,10);
				bzero(buffer,101);
			}

    		displayInfo();
    		break;
    	case 'T':
			drpdu.type = 'T';
			fflush(stdout);
			fgets(ER,10, stdin);
    		//get required info from stdin
			printf("Enter Content Name to De Register: \n");
			fflush(stdout);
			fgets(cName,10, stdin);
			strtok(cName,"\n");
    		//save info to rpdu
			strcpy(drpdu.pName,userName);
			strcpy(drpdu.cName,cName);
    		//convert Rpdu to string
			deRegPDU = convertDRpdutoString(drpdu, deRegPDU);
			//send to index server
    		write(s,deRegPDU,101);
    		//get response from index
    		receive = read(s, buffer,101);
    		//if sucessfully registered, index server returns an Ack PDU, so we let the user know and clear all variables
			if(buffer[0]==ACK_PDU_TYPE){
			printf("Conted DeRegistered Successfully!\n");
			}
			else{
				printf("ERROR: Problem Deregistering content!\n");
			}
    		displayInfo();
    		break;
    	case 'Q':
    		memset(NamePDU.data, 0, sizeof(NamePDU.data));
			NamePDU.type= 'Q';
			strcpy(NamePDU.data,userName);
			n=strlen(NamePDU.data);
			//send the file name to Index
			if( write(s, &NamePDU, n+1) <0){
				printf("\nError sending file name to Index\n");
				displayInfo();
				break;
			}
    		printf("De-registering all content\n");
   		 	//get the response from Index
			memset(data, 0, sizeof(data));
			n=read(s, data, BUFSIZE);
			IndexPDU.type = data[0];
			memset(IndexPDU.data, 0, sizeof(IndexPDU.data));
			for(i=0; i<n; i++){
				IndexPDU.data[i] = data[i+1];
			}
			IndexPDU.data[n-1] ='\0';
			if( IndexPDU.type = 'A'){
				printf("ACKNOWLEDGED!\n");
			}
    		exit(0);
  }

}
