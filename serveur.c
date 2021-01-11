#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#define MAX_BUFFER 100
#define PORT 6000


typedef struct{
    int reserve;// boolean, 0-> available; 1-> occupied
    char nom[30];
    char number[10];// number of the document
} infos;// information about a place

typedef struct{
    infos seat[10][10];
} allInfo;// information about all the places

char buffer[MAX_BUFFER];// for send() and recv();
int mysema;

// all the fonctions
void preparationServeur();
void showPlace(allInfo *places);// show all the place in concert
void reserve(int fdSocketCommunication);// reserve a place 
void readMessage(char buffer[]);// read an message
void cancel(int fdSocketCommunication);// cancel a reservation
void initializePlace(allInfo *places);
void p(int semid);//operation P for semaphore
void v(int semid); // operation V for semaphore

int main(int argc, char const *argv[]){
    preparationServeur();
}

void preparationServeur(){
    int fdSocketWaiting;// for socket()
    int fdSocketCommunication;// for accept()
    struct sockaddr_in informationServeur;
    struct sockaddr_in informationClient;
    int lengthAddress;
    int nbRecv;// for send() and recv()

    fdSocketWaiting = socket(PF_INET, SOCK_STREAM,0);
    if(fdSocketWaiting < 0){
        printf("socket incorrect\n");
        exit(EXIT_FAILURE);
    }
    else{
         printf("It\'s a serveur !\n");
    }
    //prepare local address 
    lengthAddress = sizeof(struct sockaddr_in);
    memset(&informationServeur,0x00,lengthAddress);
    informationServeur.sin_family = PF_INET;
    //all the local interface are available
    informationServeur.sin_addr.s_addr=htonl(INADDR_ANY);
    informationServeur.sin_port = htons(PORT);
    if(bind(fdSocketWaiting,(struct sockaddr *) &informationServeur,
    sizeof(informationServeur)) == -1){
        printf("error bind\n");
        exit(EXIT_FAILURE);
    }
    if(listen(fdSocketWaiting,5) == -1){
        printf("error listen\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Serveur is ready\n");
    }
    
    socklen_t tailleInfo = sizeof(informationClient);
    int pid= fork();
    if(pid!=0){
        key_t clefsem=ftok(".",20);
        mysema=semget(clefsem,1,IPC_CREAT|0666);
        semctl(mysema,0,SETVAL,1);
        if(mysema==-1){
            printf("Error semget\n");
            exit(0);
        }
        //share memory
        key_t clef=ftok(".",19);
        int shmid=shmget(clef,sizeof(allInfo),0666|IPC_CREAT);
        if(shmid==-1){
            printf("failure sharing shm!\n");
            printf("%s\n",strerror(errno));
            exit(0);
        }
        allInfo *places;
        p(mysema);
        places=shmat(shmid, NULL,0);
        if (places!=(void *)(-1)){
            initializePlace(places);
            showPlace(places);
            shmdt(places);
            v(mysema);
        }
        while(pid!=0){
            if((fdSocketCommunication = accept(fdSocketWaiting,
            (struct sockaddr *) &informationClient,&tailleInfo))== -1){
                printf("error acceptation!\n");
            }
            else{
                char *ip;
                ip=inet_ntoa(informationClient.sin_addr);
                printf("Client connected %s: %d\n",
                ip,ntohs(informationClient.sin_port));
                pid=fork();

            }
        }
    }
    if(pid==0){
        while(1){
            key_t clefsem=ftok(".",20);
            int mysema=semget(clefsem,1, IPC_CREAT|0666);
            if(mysema==1){
                printf("Error semget\n");
                exit(0);
            }
            nbRecv= recv(fdSocketCommunication,buffer,MAX_BUFFER,0);
            if(nbRecv > 0){
                if(strcmp(buffer,"1")==0){
                    reserve(fdSocketCommunication);
                }
                else if(strcmp(buffer,"0")==0){
                    cancel(fdSocketCommunication);
                }
            }
        }
    }
close(fdSocketCommunication);
close(fdSocketWaiting);
}

void readMessage(char buffer[]){
    printf("Entrer: ");
    fgets(buffer,MAX_BUFFER,stdin);
    strtok(buffer,"\n");
}

void initializePlace(allInfo *places){
    int i, j;
    for(i=0;i<10;i++){
        for(j=0;j<10;j++){
            if(i==j){
                (*places).seat[i][j].reserve=1;
            }
             else{
        (*places).seat[i][j].reserve=0;// place libre
      }
        }
    }
}

void showPlace(allInfo *places){
    int i, j;
    for(i=0;i<10;i++){
        for(j=0;j<10;j++){
            printf("%d ",(*places).seat[i][j].reserve);
        }
        printf("\n");
    }
}

void reserve(int fdSocketCommunication){
    printf("Require reservation\n");
    int nbRecv= recv(fdSocketCommunication,buffer,MAX_BUFFER,0);
    if(nbRecv > 0){
        printf("Receive a client: %s\n", buffer);
    }
    char nom[30];
    strcpy(nom,buffer);

    key_t clef=ftok(".",19);
    int shmid=shmget(clef,sizeof(allInfo),0666|IPC_CREAT);
    if(shmid==-1){
        printf("Failure of share shm!\n");
        printf("%s\n",strerror(errno));
        exit(0);
    }
    allInfo* places;
    p(mysema);
    places=shmat(shmid,NULL,0);
    if(places==(void *)(-1)){
        printf("Error sharing memory\n");
        exit(0);
    }
    int i,j;
    for(i=0;i<10;i++){
        for(j=0;j<10;j++){
            buffer[i*10+j]=(*places).seat[i][j].reserve;
        }
    }
    showPlace(places);
    shmdt(places);
    v(mysema);
    send(fdSocketCommunication,buffer,100,0);
    printf("Waiting for the answer\n");

    int occupe=1;
    while(occupe==1){
        int nbRecv= recv(fdSocketCommunication, buffer,MAX_BUFFER,0);
        if(nbRecv >0 ){
            printf("received a client\n");
        }
        char copy[5];
        strcpy(copy,buffer);
        char *line, *row;
        line=strtok(copy,", ");
        row=strtok(NULL,", ");
        i=atoi(line);
        j=atoi(row);
        p(mysema);
        places=shmat(shmid,NULL,0);
        if((*places).seat[i][j].reserve==1){
            shmdt(places);
            v(mysema);
            printf("This seat is not available, please choose another seat\n");
            strcpy(buffer,"occupe");
            send(fdSocketCommunication,buffer,100,0);
            printf("Waiting for the answer\n");
        
        }
        else{
            occupe=0;
            (*places).seat[i-1][j-1].reserve=1;
            strcpy((*places).seat[i-1][j-1].nom,nom);
            printf("Place(%d,%d) reserved\n",i,j);
            sprintf((*places).seat[i-1][j-1].number,"%d",1000000000+i*10+j);
            printf("number of the document:%s\n",(*places).seat[i-1][j-1].number);
            strcpy(buffer,(*places).seat[i-1][j-1].number);
            showPlace(places);
            shmdt(places);
            v(mysema);
            send(fdSocketCommunication,buffer,100,0);
            printf("Waiting for the answer\n");
        }
    
    }
}

void cancel(int fdSocketCommunication){
    printf("Require cancelation\n");
    strcpy(buffer,"ok");
    send(fdSocketCommunication,buffer,100,0);
    int nbRecv = recv(fdSocketCommunication,buffer,MAX_BUFFER,0);
    if(nbRecv > 0){
        printf("%s\n",buffer);
        char *nom,*number;
        nom=strtok(buffer,", ");
        number=strtok(NULL,", ");
        int i=(atoi(number)-1000000000)/10-1;
        int j=(atoi(number)-1000000000)%10-1;

        key_t clef=ftok(".",19);
        int shmid=shmget(clef,sizeof(allInfo),0666|IPC_CREAT);
        if(shmid==-1){
            printf("Failure of sharing shm\n");
            printf("%s\n",strerror(errno));
            exit(0);
        }
        allInfo*places;
        p(mysema);
        places=shmat(shmid,NULL,0);
        if(shmat(shmid,NULL,0)==(void *)(-1)){
            printf("Failure sharing memory\n");
            exit(0);
        }
        if(strcmp((*places).seat[i][j].nom,nom)==0 &&
        strcmp((*places).seat[i][j].number,number)==0){
            (*places).seat[i][j].reserve=0;
            strcpy((*places).seat[i][j].nom,"");
            strcpy((*places).seat[i][j].number,"");
            showPlace(places);
            printf("Seat(%d,%d) is canceled\n", i+1,j+1);
            shmdt(places);
            v(mysema);
            strcpy(buffer,"ok");
            send(fdSocketCommunication,buffer,100,0);
        }
        strcpy(buffer,"failure");
        send(fdSocketCommunication,buffer,100,0);
        
            }
}

void p(int semid){
    int rep;
    struct sembuf sb={0,-1,0};
    rep=semop(semid,&sb,1);
}
void v(int semid){
    int rep;
    struct sembuf sb={0,1,0};
    rep=semop(semid,&sb,1);
}



