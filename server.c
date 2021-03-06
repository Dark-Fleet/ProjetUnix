/*
	File name: 	server.c
	Author: 	BERGE Benjamin, LAMBRICHT Antoine
	Serie:		1
*/
#include "server.h"

int quitint;
int timer_is_over;
int shmid;
int playing;

void timer_handler(int signal){
	if (signal == SIGALRM) {
		timer_is_over = TRUE;
		fprintf(stderr,"Les %d seconde sont finie\n",ALARM);
	}
}

void quit_handler(int signal){
	if (signal == SIGINT) {
		quitint = TRUE;
	}
}

void shutdown_server(player players[MAX_PLAYERS],int server_socket){
	printf("Server shutdown...\n");
	deleteSharedMemory(shmid);
	viderPlayer(players);
	close(server_socket);
	exit(0);
}

void restart(player players[MAX_PLAYERS],int* playerCount){
	printf("Server restarting...\n");
	viderPlayer(players);
	playing = FALSE;
	*playerCount = 0;
	struct sigaction timer;
	timer.sa_handler = &timer_handler;
	sigemptyset(&timer.sa_mask);
	sigaction(SIGALRM, &timer, NULL);
	printf("Server restarted!\n");
}

void viderPlayer(player players[MAX_PLAYERS]){
	int i;
	for(i=0;i<MAX_PLAYERS;i++){
		if(players[i].socket != 0){
			SYS(close(players[i].socket));
		}
	}
	memset(players,0,sizeof(player)*MAX_PLAYERS);
}

int main(int argc,char** argv){
	/*char buffer[256];*/
	Card cartes[NB_CARDS];
	Card ecarts[MAX_PLAYERS][SIZE_ECART];
	player players[MAX_PLAYERS] = {{0}};
	int server_socket,port,highestFd,playerCount = 0;
	int game_state = 0;
	int turn_state = 0;
	int player_turn_state = 0;
	int ecartCount = 0;
	struct timeval timeout;
	struct sigaction timer,quit;
	fd_set fdset;
	Message message;
	Card pli[MAX_PLAYERS];
	int player_to_play;
	int first_player;
	int turnCounterCard;
	int turnCounter;
	int player_turn_counter;
	int round_counter;
	
	
	

	initCartes(cartes);
	

	if( argc != 2 ){
		fprintf(stderr,"Usage: %s port\n",argv[0]);
		exit(1);
	}
	/*initialise le server pour qu'il réagisse au signal SIGALARM*/
	timer.sa_handler = &timer_handler;
	sigemptyset(&timer.sa_mask);
	sigaction(SIGALRM, &timer, NULL);
	
	/*initialise le server pour qu'il réagisse au signal SIGALARM*/
	quit.sa_handler = &quit_handler;
	sigemptyset(&quit.sa_mask);
	sigaction(SIGINT, &quit, NULL);

	port = atoi(argv[1]);

	initiateServer(&server_socket,port);
	
	shmid = initSharedMemory(TRUE);
	init_semaphore(TRUE);

	


	while(TRUE){
		if(quitint){
			shutdown_server(players,server_socket);
		}
		int action,i,j;


		FD_ZERO(&fdset);
		FD_SET(server_socket,&fdset);
		highestFd = server_socket+1;
		for (i = 0; i < MAX_PLAYERS; i++) {
			if (players[i].socket > 0) {
				FD_SET(players[i].socket, &fdset);
			}
			if (players[i].socket >= highestFd) {
				highestFd = players[i].socket+1;
			}
		}

		timeout.tv_sec  = 2;
		timeout.tv_usec = 500000;

		action = select(8,&fdset,NULL,NULL,&timeout);

		if(action >0){
			//si le socket d'inscription est modifié
			if(FD_ISSET(server_socket,&fdset)){
				
				struct sockaddr_in clientAdress;
				int len = sizeof(clientAdress);
				
				int clientSkt = accept(server_socket,(struct sockaddr *)&clientAdress,(socklen_t*)&len);
			
				fprintf(stderr,"Connection from %s %d\n",inet_ntoa(clientAdress.sin_addr), ntohs(clientAdress.sin_port));
			
				
				
				if(playerCount<MAX_PLAYERS && !playing){
					//inscription de l'utilisateur
					Message msg;
					Message nameTaken;
					if(receive_msg(&msg,clientSkt)){
						if(msg.action == INSCRIPTION){
							int i,nameOK;
							nameOK = TRUE;
							//test si un joueur à déjà ce nom
							for(i=0;i<playerCount;i++){
								if(strcmp(players[i].name,msg.payload.name)==0){
									nameOK = FALSE;
									break;
								}
							}
							//si le nom est bon , on enregistre le joueur 
							if(nameOK){
								strcpy(players[playerCount].name, msg.payload.name);
								players[playerCount++].socket = clientSkt;
								
								
								ecrirePlayers(players,playerCount);
								fprintf(stderr,"Player %s à été inscrit (socket : %d) \n",players[playerCount-1].name,players[playerCount-1].socket);
								
								if(playerCount == 1){
									fprintf(stderr,"Attente d'autres joueur, la partie va commencer dans %d secondes\n",ALARM);
									alarm(ALARM);
								}
							}							
							//sinon on lui envoie un message
							else{
								nameTaken.action = INSCRIPTIONKO;
								strcpy(nameTaken.payload.str,"Inscription refusé : Nom déjà pris\n");
								send_message(nameTaken,clientSkt);
								close(clientSkt);
							}
						}
					}else{
						close(clientSkt);
					}
					
				}else{
					message.action = INSCRIPTIONKO;
					strcpy(message.payload.str,"Impossible de s'inscrire pour le moment\n");
					send_message(message,clientSkt);
				}
			}
			// pour chaque joueur enregistré , on test si son socket à été modifié
			for (i = 0; i < playerCount; i++) {
				if (FD_ISSET(players[i].socket, &fdset)) {
					//si oui , on lis le message 
					Message msg;
					Message distribution_ecart;
					if (receive_msg(&msg, players[i].socket)) {					
						switch(msg.action){
							
							case ENVOI_ECART:
								memcpy(&ecarts[i],msg.payload.ecart,sizeof(Card)*SIZE_ECART);
								ecartCount++;
								fprintf(stderr,"Ecart reçu de %s\n",players[i].name);
								if(ecartCount == playerCount){
									/*distribution des écarts*/
									fprintf(stderr,"Tout les écart on été reçu\n");
									for (j = 0; j < playerCount; j++) {
										distribution_ecart.action = DISTRIBUTION_ECART;
										if(j==playerCount-1){
											memcpy(distribution_ecart.payload.ecart,&ecarts[0],sizeof(Card)*SIZE_ECART);
										}else{
											memcpy(distribution_ecart.payload.ecart,&ecarts[j+1],sizeof(Card)*SIZE_ECART);
										}
										send_message(distribution_ecart,players[j].socket);
										ecartCount = 0;
										game_state = 2;
										
									}
								}
								break;
							case REPONSE_CARTE:
								
								pli[i]=msg.payload.carte;
								ecrirePlis(pli);
								//send to all player that the pli have changed in the shared memory
								int k;
								Message m;
								m.action = PLI_UPDATE;
								for(k = 0;k<playerCount;k++){
									send_message(m,players[k].socket);
								} 								
								player_turn_state = END_PLAYER_TURN;
								break;
							case REPONSE_POINTS:
								players[i].points += msg.payload.points;
								ecrirePlayers(players,playerCount);
								break;
							default:
								perror("action invalide\n");
								exit(1);
						}
					} else {
						if(playing){
							restart(players,&playerCount);
							game_state = START_ROUND;
							turn_state = INIT_TURN;
							player_turn_state = SEND_DEMEND;
							
						}else{
							fprintf(stderr,"Removing player \n");
							removePlayer(players,&playerCount,i);
							for (j = 0; j < MAX_PLAYERS; j++){
								fprintf(stderr,"Player %d -> %s \n",j,players[j].name);
							}
						}
						
					}

				}
			}
		}
		if(playing){
			switch(game_state){
				
				case START_ROUND:
					melanger(cartes);
					distribution(players,playerCount,cartes);
                    game_state=WAIT_FOR_ECART;
					first_player = 0;
					turnCounterCard = NB_CARDS/playerCount;
					turnCounter = MAX_TURN;
					break;
				case WAIT_FOR_ECART: 
				
					break;
				case RANDOM_PAPAYOO:
					
					srand(time(NULL));
					int i;
					int couleur = (rand()%4)+1;
					Message m;
					m.action = PAPAYOO;
					m.payload.papayoo = couleur;
					for(i = 0;i<playerCount;i++){
						send_message(m,players[i].socket);
					}
					game_state = TURN;
					break;
				case TURN:
				
					switch(turn_state){
						case INIT_TURN:
						
							player_to_play = first_player;
							
							/*test*/
							memset(&pli,0,sizeof(Card)*playerCount);
							ecrirePlis(pli);
							player_turn_counter = playerCount;
							turn_state = START_TURN;
							break;
						case START_TURN:
						
							switch(player_turn_state){
								case SEND_DEMEND:;
									Message demandeCarte;
									demandeCarte.action = DEMANDE_CARTE;
									send_message(demandeCarte,players[player_to_play].socket);
									player_turn_state = WAIT_RESPONSE;
									break;
								case WAIT_RESPONSE:
								
									break;
								case END_PLAYER_TURN:
								
									/*si tout les joueurs on joué, fin du tour*/
									player_turn_counter--;
									if(player_turn_counter == 0){
										turn_state = END_TURN;
									}else{
										player_to_play++;
										if(player_to_play == playerCount){
											player_to_play = 0;
										}
										
									}
									player_turn_state = SEND_DEMEND;
									break;
								
							}
							break;
						case END_TURN:
							turnCounterCard--;
							turnCounter--;
							int j;
							int couleur = pli[first_player].couleur;
							int max = 0;
							int looser;
							/*ici on décide qui à perdu*/
							for(j=0;j<playerCount;j++){
								if(pli[j].couleur == couleur){
									if(pli[j].num > max){
										max = pli[j].num;
										looser=j;
									}
								}
							}
							/*Creation du message*/
							Pli pli_to_send;
							pli_to_send.nbr = playerCount;
							/*pli_to_send.pli = pli;*/
							Message envoiPli;
							envoiPli.action = ENVOI_PLI;
							envoiPli.payload.pli = pli_to_send;
							
							send_message(envoiPli,players[looser].socket);
							if(turnCounterCard == 0 || turnCounter == 0){
								game_state = END_ROUND;
							}else{
								player temp[MAX_PLAYERS];
								int j=looser;
								for(i=0;i<playerCount;i++){
									temp[i]=players[j];
									j++;
									if(j==playerCount){
										j = 0;
									}
								}
								memcpy(players,temp,sizeof(player)*MAX_PLAYERS);
								
							}
							turn_state = INIT_TURN;
							break;
							
					}
					break;
				case END_ROUND:
					round_counter--;
					for(i = 0;i<playerCount;i++){
						Message demandePoint;
						demandePoint.action = DEMANDE_POINTS;
						send_message(demandePoint,players[i].socket);
					}
					
					if(round_counter == 0){
						
						for(i = 0;i<playerCount;i++){
							Message alerteFinPartie;
							alerteFinPartie.action = ALERTE_FIN_PARTIE;
							send_message(alerteFinPartie,players[i].socket);
						}
						restart(players,&playerCount);
					}
					game_state = START_ROUND;
					break;
				default:
					break;
			}			
		}else{
			if(timer_is_over){
				if(playerCount>=2){
					playing = TRUE;
					round_counter = MAX_ROUND;
				}else{
					restart(players,&playerCount);
				}
				timer_is_over = FALSE;
			}
			
		}

	}
    close(server_socket);
	return EXIT_SUCCESS;
}

void removePlayer(player players[],int* playerCount,int index){
	int j;
	close(players[index].socket);
	players[index].socket = 0;
	memcpy(players[index].name, "\0", NAME_SIZE);
	(*playerCount)--;

	for (j = index+1; j <= *playerCount; j++) {
		players[j-1].socket = players[j].socket;
		sprintf(players[j-1].name, "%s", players[j].name);
	}
	players[j-1].socket = 0;
	memcpy(players[j-1].name, "\0", NAME_SIZE);
	ecrirePlayers(players,*playerCount);

}


int receive_msg(Message *msg, int fd) {
	int bytes_received;
	if ((bytes_received = recv(fd, msg, sizeof(Message), 0)) <= 0) {
		if (bytes_received == 0) {
			printf("Client disconnected.\n");
		}
		else {
			perror("Could not receive message");
		}
		return FALSE;
	}
	return TRUE;
}

void initCartes(Card cartes[]){
	int i,c = 1;
	for(i=1;i<=NB_CARDS;i++){
		if(c<PAYOO){
			cartes[i-1].couleur = c;
			
			if(i%10 == 0){
				c+=1;
				cartes[i-1].num = 10;
			}else{
				cartes[i-1].num = (i%10);
			}
		}else{
			cartes[i-1].num = (i-40);
			cartes[i-1].couleur = c;
		}
		
	}
}

void melanger(Card cartes[]){
	srand(time(NULL));
    size_t i;
    for (i = 0; i < NB_CARDS-1; i++) {
		size_t j = i + rand() / (RAND_MAX / (NB_CARDS - i) + 1);
        Card t = cartes[j];
        cartes[j] = cartes[i];
        cartes[i] = t;
	}
    
}

void distribution(player players[],int playerCount,Card cartes[]){
    Message deck;
    Dist dist;
    Card *cartes_player;
	int i, n = NB_CARDS/playerCount;
    int x,j=0;
    cartes_player=malloc(n*sizeof(Card));
        
    for(i=0;i<playerCount;i++){
        memcpy(cartes_player,cartes+j,n*sizeof(Card));
        dist.nbr=n;
        for(x=0;x<n;x++){
            dist.cards[x]=cartes_player[x];
        }
        deck.action = DISTRIBUTION;
        deck.payload.dist =dist;
        send_message(deck,players[i].socket);
        j+=n;
    }
}
