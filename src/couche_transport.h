#ifndef __COUCHE_TRANSPORT_H__
#define __COUCHE_TRANSPORT_H__

#include <stdint.h> /* uint8_t */
#include <stdbool.h> /* bool */
#define MAX_INFO 96

/*************************
* Structure d'un paquet *
*************************/

typedef struct paquet_s {
    uint8_t type;         /* type de paquet, cf. ci-dessous */
    uint8_t num_seq;      /* numéro de séquence */
    uint8_t lg_info;      /* longueur du champ info */
    uint8_t somme_ctrl;   /* somme de contrôle */
    unsigned char info[MAX_INFO];  /* données utiles du paquet */
} paquet_t;

/******************
* Types de paquet *
******************/
#define DATA          1  /* données de l'application */
#define ACK           2  /* accusé de réception des données */
#define NACK          3  /* accusé de réception négatif */
#define CON_REQ       4  /* demande d'établissement de connexion */
#define CON_ACCEPT    5  /* acceptation de connexion */
#define CON_REFUSE    6  /* refus d'établissement de connexion */
#define CON_CLOSE     7  /* notification de déconnexion */
#define CON_CLOSE_ACK 8  /* accusé de réception de la déconnexion */
#define OTHER         9  /* extensions */


/* Capacite de numerotation */
#define SEQ_NUM_SIZE 16

typedef enum {
    ETAT_NONENVOYE,
    ETAT_ENVOYE,
    ETAT_CONFIRME,
} etat_paquet_t;

#define ETAT_NONRECU ETAT_NONENVOYE
#define ETAT_RECU ETAT_ENVOYE

/* ************************************** */
/* Fonctions utilitaires couche transport */
/* ************************************** */

int calcul_checksum(paquet_t* p);
bool check_integrity(paquet_t* p);

/*--------------------------------------*
* Fonction d'inclusion dans la fenetre *
*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille);

unsigned int increment(unsigned int n);

#endif
