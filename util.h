/*
	File name: 	util.h
	Author: 	BERGE Benjamin, LAMBRICHT Antoine
	Serie:		1
*/

#if ! defined (_UTIL_H_)
#define _UTIL_H_

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include	<unistd.h>


#define SYS(call) ((call) == -1) ? (perror(#call ": ERROR"),exit(1)) : 0

#define TRUE 1
#define FALSE 0

#define OK 0
#define KO 1

#define NB_CARDS 60
#define MAX_PLAYERS 4


#define NAME_SIZE 40
#define MESSAGE_SIZE 255

#define INSCRIPTION 0
#define NAME_TAKEN 1
#define INSCRIPTIONKO 2

#define PIQUE 1
#define TREFLE 2
#define CARREAU 3
#define COEUR 4
#define PAYOO 5

typedef struct card{
  unsigned char num;
  unsigned char couleur;
}Card;

typedef struct player {
	int socket;
	char name[NAME_SIZE];
	int points;
} player;
/*Union of what a payload can be*/
union Payload{
	/*distribution*/
	Card dist30[30];
	Card dist20[20];
	Card dist15[15];
	
	/*ecart*/
	Card ecart[5];
	
	/*jouer une carte*/
	Card carte;
	
	/*Message*/
	char str[MESSAGE_SIZE];
	
	/*Name pour inscription*/

	char name[NAME_SIZE];
};

typedef struct message{
	int action;
	union Payload payload;
}Message;


#endif
