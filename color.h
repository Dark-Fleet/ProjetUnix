/*
	File name: 	color.h
	Author: 	BERGE Benjamin, LAMBRICHT Antoine
	Serie:		1
*/

#include <stdio.h>
#include <stdlib.h>

#define PIQUE_C  "\xE2\x99\xA0"
#define TREFLE_C "\xE2\x99\xA3"
#define COEUR_C  "\xE2\x99\xA5"
#define CARREAU_C  "\xE2\x99\xA6"

// un peu de couleur

#define noirTexte 30
#define rougeTexte 31
#define vertTexte 32
#define jauneTexte 33
#define bleuTexte 34
#define magentaTexte 35
#define cyanTexte 36
#define blancTexte 37

#define noirFond 40
#define rougeFond 41
#define vertFond 42
#define jauneFond 43
#define bleuFond 44
#define magentaFond 45
#define cyanFond 46
#define blancFond 47


#define couleurOn(A,B,C) printf("\033[%d;%d;%dm",A,B,C)
#define couleurOff() printf("\033[0m");

#include "util.h"

void print_color(Card card);
void print_tab_color(Card *cards,int nb);