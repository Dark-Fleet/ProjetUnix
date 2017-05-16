/*
	File name: 	joueur.c
	Author: 	BERGE Benjamin, LAMBRICHT Antoine
	Serie:		1
*/
#include "joueur.h"

Card* our_cards;
int main(int argc,char** argv){
	int client_socket,port;
	struct hostent *host;
    
	if( argc != 3 ){
		   fprintf(stderr,"Usage: %s ip port\n",argv[0]);
        exit(1);
    }

	if ((host=gethostbyname(argv[1])) == NULL) {
		perror("Client: gethostbyname failed");
		exit(EXIT_FAILURE);
	}
	port = atoi(argv[2]);

	initSharedMemory(FALSE);
	init_semaphore(FALSE);

	//initialise la connection et l'inscription
	Message msg = inscription();
	initiateConnection(&client_socket,host,port,msg);
	fprintf(stderr,"Connected\n");
	
	while(TRUE){
		get_request(client_socket);
	}
	close(client_socket);
	return EXIT_SUCCESS;
}

int receive_msg(Message *msg, int fd) {
	int bytes_received;
	if ((bytes_received = recv(fd, msg, sizeof(Message), 0)) <= 0) {
		if (bytes_received == 0) {
			printf("Server disconnected.\n");
		}
		else {
			perror("Could not receive message\n");
		}
		return FALSE;
	}
	return TRUE;
}



void get_request(int server_socket){
	Message msg;
	if (receive_msg(&msg,server_socket)){
		fprintf(stderr,"Message action from server :%d\n",msg.action);
		
		switch(msg.action){
			case INSCRIPTIONKO:
				fprintf(stderr,"%s",msg.payload.str);
				exit(1);
            case DISTRIBUTION:
                register_cards(msg,server_socket);
                break;
			case DISTRIBUTION_ECART:
				/*TODO add ecart to cards*/
				
				break;
			case DEMANDE_CARTE:
				//TODO
				break;
			case DEMANDE_POINTS:
				//TODO
				break;
			case ENVOI_PLI:
				//TODO
				break;
			case ALERTE_FIN_PARTIE:
				//TODO
				break;
			default:
				perror("action invalide");
                exit(1);
		}
	}else{
		exit(1);
	}

}

Message inscription(){
	char name[NAME_SIZE];
	Message inscription;
	
	printf("Enter your name > ");
	scanf("%s", name);
	fflush(stdin);
	fflush(stdout);
				
	inscription.action = INSCRIPTION;
	strcpy(inscription.payload.name,name);
				
	return inscription;
}

void register_cards(Message msg, int socket){
    Message m;
    Dist deck= msg.payload.dist;
    int nbr = deck.nbr;
    our_cards=deck.cards;
    print_tab_color(our_cards,nbr);
    lire_remove_emplacements(m.payload.ecart,our_cards,&nbr,SIZE_ECART);
    m.action=ENVOI_ECART;
   
    send_message(m,socket);
    
}


void lire_remove_emplacements(Card * buffer,Card * source,int *size,int nbr){
    char l[nbr*2];
    Card new_source[*size-nbr];
    int i,j,x,tab[nbr];
    int invalide=FALSE;
    char * token;
    /*Entrée des cartes*/
    printf("Nous allez maintenant choisir l'écart.\nEntrer l'emplacement des %d cartes de l'écart (en commancant par 1)\n",SIZE_ECART);
    printf("usage->1-2-3 ...\n");
    do{
        if(invalide){
            printf("\nInvalide, recommencer\n");
        }
        invalide=FALSE;
        scanf("%s",l);
        printf("\n%s\n",l);
        token=strtok(l,"-");
        tab[0]=atoi(token);
        i=1;
        
        /*validation du bon format*/
        while(i<nbr && token!=NULL){
            token=strtok(NULL,"-");
            if(token!=NULL){
                tab[i]=atoi(token);
                i++;
            }
        }
        if(i!=6){
            invalide=TRUE;
            break;
        }
        
        /*Verification des doublons et de l'absence de la carte*/
        for(i=0;i<nbr;i++){
            for(j=i;j<nbr;j++){
                if(tab[i]==tab[j] || tab[i]>*size){
                    invalide=TRUE;
                }
            }
        }
    
    }while(invalide);
    
    /*copie de l'ecart dans le buffer*/
    for(i=0;i<nbr;i++){
        buffer[i]=source[tab[i]];
    }
    /*réécriture de la source sans l'ecart*/
    *size=*size-nbr;
    j=0;
    i=0;
    x=0;
    while(i<*size){
        if(j==tab[x]){
            j++;
            x++;
        }else{
            new_source[i]=source[j];
            i++;
            j++;
        }
    }
    source=new_source;
    
}
