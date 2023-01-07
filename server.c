#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>



// General PDU structure
struct pdu {
char type;
char data[100];
};

// PDU Structure for Registering Content
struct RPDU{ 
	char type;
    char pName[10];
    char cName[10];
    char address[80];
};

// PDU Structure for Deregistering Content
struct DRPDU{ 
	char type;
    char pName[10];
    char cName[10];
};

//convert RPDU string from peer to RPDU struct format
struct RPDU rpduStringToStruct(char *stringRPDU,struct RPDU*rpdu){
	memset(rpdu->pName, '\0', 10);
	memset(rpdu->cName, '\0', 10);
	memset(rpdu->address, '\0', 80);

    strcat(stringRPDU,",");
	rpdu->type = stringRPDU[0];
    char *token=strtok(stringRPDU,",");
    token=strtok(NULL,",");
	strcpy(rpdu->pName,token);
    token=strtok(NULL,",");
	strcpy(rpdu->cName,token);
    token=strtok(NULL,",");
	strcpy(rpdu->address,token);

    return *rpdu;
};

//convert DRPDU string from peer to DRPDU struct format
struct DRPDU drpduStringToStruct(char *stringDRPDU,struct DRPDU*drpdu){

	memset(drpdu->pName, '\0', 10);
	memset(drpdu->cName, '\0', 10);

    strcat(stringDRPDU,",");
    drpdu->type=stringDRPDU[0];
    char *token=strtok(stringDRPDU,",");
    token=strtok(NULL,",");
    strcpy(drpdu->pName,token);
    token=strtok(NULL,",");
    strcpy(drpdu->cName,token);

    return *drpdu;
};

int
main(int argc, char *argv[])
{
	#define ACK "A,Registered"
	#define MSGSIZE 100
	#define ERROR "E,ERROR"
	struct sockaddr_in fsin; //peer address
	struct sockaddr_in sin; // IP address
    //s is socket descriptor
	int alen, s, type; //type is socket type
	int port = 3000;
	struct pdu pduIn;
	struct pdu  pduOut;
	char	*message;
	
	message = malloc(101);
    struct RPDU rpdu;//PDU to register files
    struct DRPDU drpdu;//PDU to deregister files

	char *data[10][4];//Data array to store peer, content, peer addr, flag
	int i, j, check, index;
	index = 0;
	check = -1;
	for(i = 0;i < 10;i++){
		for(j = 0;j < 4;j++){
			//data[i][j]='\0';
			data[i][j]=malloc(20);
			memset(data[i][j], 0, sizeof(data[i][j]));
		};
	};

	char peerMsg[101];
	memset(peerMsg,0,strlen(peerMsg));

	switch(argc){
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Cannot create socket\n");

    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Cannot bind to port\n");
        listen(s, 5);
	alen = sizeof(fsin);
	
	while (1) {
		memset(&peerMsg, 0 , sizeof(peerMsg));
	
		//Get incoming message from the peer over UDP
		recvfrom(s, peerMsg, MSGSIZE, 0,(struct sockaddr *)&fsin, &alen);
		strcpy(message,peerMsg);
		fflush(stdout);//clear terminal output

		memset(&pduIn.data, 0,sizeof(pduIn.data));

		//Take the peer message and store it into the pduIn
		pduIn.type = peerMsg[0];
		for (i = 0; i < MSGSIZE; i++) {
			pduIn.data[i] = peerMsg[i + 1];
		}
		//Check what to do with the PDU depending on type
		switch(pduIn.type){
		case 'S':
			printf("Looking if the requested file exists on server: %s\n", pduIn.data);
			strcat(pduIn.data,"\n");
			//send back the port number of where the requested content is
			check = findContentLocation(data, pduIn.data);
			if(check != -1){
				memset(&pduOut.data, 0 , sizeof(pduOut.data));
				pduOut.type = 'S';
				strcpy(pduOut.data, data[check][2]);
				sendto(s, &pduOut, strlen(pduOut.data)+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
				//Track usage of content
				//Add 1 each time it is used
				int num;
				num = atoi(data[check][3]);
				num++;
				sprintf(data[check][3],"%d",num);
				}

			else {//Send back an error if the content does not exist
				pduOut.type = 'E';
				strcpy(pduOut.data, "That content is not found");
				sendto(s, &pduOut, strlen(pduOut.data)+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
			}
			break;
			
		case 'O'://if the PDU type is 'O' send list of registered content
			memset(&pduOut.data, 0 , sizeof(pduOut.data));
			pduOut.type = 'O';
			//Get stored content and put it it into the pduOut
			for(i=0; i<10;i++){
				if(strcmp(data[i][1],"") != 0){
					strcat(pduOut.data, data[i][1]);
				}
			}
			if(strcmp(pduOut.data,"") == 0){// if there is no regidtered data
				strcpy(pduOut.data, "There is not registered data");
			}
			sendto(s, &pduOut, strlen(pduOut.data) + 1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
			break;
		
		case 'R'://if the PDU type is 'R' then register content
		        rpdu = rpduStringToStruct(message,&rpdu);//Change the string from message to rpdu
		        strcat(rpdu.cName,"\n");
		        printf("The peer: %s and content: %s has been registered\n",rpdu.pName,rpdu.cName);
				//check if a peer and file by that name already exist
		        if (checkDulicate(data,rpdu.pName,rpdu.cName) == 1){
		    	    sendto(s, ACK, strlen(ACK), 0, (struct sockaddr *) &fsin, sizeof(fsin));

		    	    memcpy(data[index][0],rpdu.pName,sizeof(rpdu.pName));//add data to the array
		    	    memcpy(data[index][1],rpdu.cName,sizeof(rpdu.cName));
		    	    memcpy(data[index][2],rpdu.address,sizeof(rpdu.address));
	    	     	index++;//increase array index
		    	 }
		    	 else{//if there is a duplicate peer and content then send Error PDU
		    	     sendto(s, ERROR, strlen(ERROR), 0, (struct sockaddr *) &fsin, sizeof(fsin));
		    	 }

			break;
		case 'T'://if the PDU type is 'T' then deregister content
		        drpdu = drpduStringToStruct(message,&drpdu);
		        strcat(drpdu.cName,"\n");
		        printf("Removing content from server: %s, %s\n",drpdu.pName,drpdu.cName);
		        int dIndex;
				//find the location of the content to deregister in data array
				dIndex = findDeregistrationLocation(data,drpdu.pName,drpdu.cName);
		        if(dIndex == -1){//Send error if content is not found in data array
		            sendto(s, ERROR, strlen(ERROR), 0, (struct sockaddr *) &fsin, sizeof(fsin));
		        }
		        else{//Delete content in array and send acknowledgement PDU
				bzero(data[dIndex][0],sizeof(data[dIndex][0]));
				bzero(data[dIndex][1],sizeof(data[dIndex][1]));
				bzero(data[dIndex][2],sizeof(data[dIndex][2]));
				sendto(s, ACK, strlen(ACK), 0, (struct sockaddr *) &fsin, sizeof(fsin));
		        }
			break;

			case 'Q'://if the PDU type is 'Q' then deregister all content and end program
			printf("Removing: %s\n",pduIn.data);
			for(i=0; i<10;i++){//remove all content from data array
				if( strcmp(data[i][1],"")!=0 && strcmp(data[i][0],pduIn.data) == 0){
				memset(data[i][0],0,sizeof(data[i][0]));
				memset(data[i][1],0,sizeof(data[i][1]));
				memset(data[i][2],0,sizeof(data[i][2]));
				memset(data[i][3],0,sizeof(data[i][3]));
				}
			}
			memset(&pduOut.data, 0 , sizeof(pduOut.data));
			pduOut.type = 'O';
			sendto(s, &pduOut, strlen(pduOut.data) + 1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
			break;
		}
	}
}

//Make sure the file and peer do not already exist
int checkDulicate(char *data[10][4],char*pName,char*cName){
    int i, flag;
    flag = 1;//flag is 1 when there is not matching peer and content
    for(i = 0;i < 10;i++){
        if((strcmp(cName,data[i][1]) == 0) && (strcmp(pName,data[i][0]) == 0)){
            flag = 0; //if there is a matching peer and content then set flag to 0
        }
    }
    return(flag);
};

//Find the location of content and peer to deregister in data array
int findDeregistrationLocation(char *data[10][4],char*pName,char*cName){
    int i;
    int index = -1;
    for(i=0;i<10;i++){
        if((strcmp(pName,data[i][0]) == 0)&&(strcmp(cName,data[i][1]) == 0)){
            index = i;
        }
    }
    return(index);//return loaction in array
};

//Return location of content in data array
int findContentLocation(char *data[10][4],char*cName){
    int i, index;
    index = -1;
    for(i = 0;i < 10;i++){
        if((strcmp(cName,data[i][1]) == 0) && index == -1){
            index = i;
        }
        else if((strcmp(cName,data[i][1]) == 0) && index != -1){
            if(atoi(data[i][3]) < atoi(data[index][3])){
                index = i;
            }
        }
    }
    return(index);
};
