#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sem.h>

#define MAX_BUFFER 100
#define PORT 6000

char buffer[MAX_BUFFER];// for send() and recv();
int place[10][10];// place for the concert, in 0 and 1

void preparationClient();// configuration for a client
void showPlace();// show all the place in concert
void reserve(int fdSocket);// reserve a place 
void readMessage(char buffer[]);// entre an message
void cancel(int fdSocket);// cancel a reservation

int main(int argc, char const *argv[]){
    preparationClient();
}

void preparationClient(){
    int fdSocket;
    int nbRecv;
    struct sockaddr_in informationServeur;
    int lengthAddress;

    fdSocket=socket(AF_INET, SOCK_STREAM,0);
    if(fdSocket < 0){
        printf("socket incorrect!\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("It\'s a client !\n");
        }
// prepare serveur information 
    lengthAddress=sizeof(struct sockaddr_in);
    memset(&informationServeur,0x00,lengthAddress);
    informationServeur.sin_family=PF_INET;
    //serveur address
    inet_aton("127.0.0.1",&informationServeur.sin_addr);
    // all the interface are available
    informationServeur.sin_port=htons(PORT);
    if(connect(fdSocket,(struct sockaddr *)&informationServeur,sizeof(informationServeur))== -1){
        printf("Connection impossible!\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Connextion succeed!\n");
    }

    printf ("Dear customer   :-) \nWelcome to our reservation page for the Taylor Swift Reputation concert!\n\n");
    while(1){
        printf("Do you want to make a reservation(1) or cancel a reservation(0)?\n");
        printf("enter 8 to quit\n");
        readMessage(buffer);
        if(strcmp(buffer,"1")==0){// reservation
            reserve(fdSocket);
        }
        else if(strcmp(buffer,"0")==0){//cancel a reservation
            cancel(fdSocket);
        }
        else if (strcmp(buffer,"8")==0){ // quit this program
            close(fdSocket);
            printf("Hava a nice day, goodbye!\n");
            exit(0);
        }
    }


}


void showPlace(){
  int i, j;
  printf("    1 2 3 4 5 6 7 8 9 10\n");
  for(i=0;i<10;i++){
    if(i==9){
      printf("10| ");
    } else {
      printf(" %d| ", i+1);
    }
    for(j=0;j<10;j++){
      printf("%d ", place[i][j]);
    }
    printf("\n");
  }
}
void readMessage(char buffer[]){
    printf("Enter:");
    fgets(buffer,MAX_BUFFER,stdin);
    strtok(buffer,"\n");

}
void cancel(int fdSocket){
    strcpy(buffer,"0");
    send(fdSocket,buffer,100,0);//send "0" to serveur for reservation
    printf("Hold on for one second please\n");
    int nbRecv=recv(fdSocket, buffer,MAX_BUFFER,0);
    if(nbRecv>0 && strcmp(buffer,"ok")==0){
        printf("Please enter your name and the number of the document: name,number (no parentheses)\n");
        strcpy(buffer,"");
        readMessage(buffer);
        send(fdSocket,buffer,100,0);
        printf("Waiting...\n");
    }
    nbRecv = recv(fdSocket, buffer,MAX_BUFFER,0);
    if(nbRecv>0){
        if(strcmp(buffer,"ok")==0){
            printf("cancel succeed!\n");
        }
        else if(strcmp(buffer,"failure")==0){
            printf("not able to cancel this time, please try again later.\n");
        }
    }
    
}
void reserve(int fdSocket){
    send(fdSocket,buffer,100,0); // send 1 to serveur for reservation
    printf("Wait for the answer\n");
    printf("Please enter your name\n");
    readMessage(buffer);
    send(fdSocket,buffer,100,0);//send the name to serveur for reservation

    int nbRecv=recv(fdSocket,buffer,MAX_BUFFER,0);
    if(nbRecv>0){
        printf("Here are the places in this concert:(0 means available,1 means occupied\n");
        for(int i=0;i<10;i++){
            for(int j=0;j<10;j++){
            place[i][j]=buffer[i*10+j];// refresh place information
        }
     }
    }
    
    showPlace();// show places
    printf("Which place would you like to reserve? Par excemple:1,2 (no parentheses)\n");
    int occupation=1;
    if(occupation==1){//repeat when the place is not available
        readMessage(buffer);
        char copy[5];
        strcpy(copy,buffer);// copy buffer
        char *line, *row;
        line=strtok(copy,",");
        row=strtok(NULL,",");
        int i=atoi(line);
        int j=atoi(row); //transfer char into int
        if(place[i-1][j-1]==1){
            printf("Sorry, this place is not available, please choose another one:\n");
        }
        else{
            occupation=0;//quit the repeatation 
            printf("Reserving, please hold on a second\n");
            send(fdSocket,buffer,100,0);// send information about the place
            int nbRecv = recv(fdSocket,buffer,MAX_BUFFER,0);
            if(nbRecv>0){
                if(strcmp(buffer,"occupation")==0){
                    printf("Sorry, this place is not available, please choose another one:\n");
                }else{
                    printf("The place(%d,%d) is reserved ! Thank you for your patience.\n",i,j);
                    printf("The number of your document is %s\n",buffer);
                }
            }
        }
    }
    

}

