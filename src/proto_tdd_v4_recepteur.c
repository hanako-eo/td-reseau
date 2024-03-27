/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v0                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"


void petat(etat_paquet_t send_paquets_state[SEQ_NUM_SIZE]) {
    for (int i = 0; i < SEQ_NUM_SIZE; i++) {
        printf(" %d |", send_paquets_state[i]);
    }
    printf("\n");
}

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquets[SEQ_NUM_SIZE]; /* paquet utilisé par le protocole */
    paquet_t paquet; /* paquet utilisé par le protocole */
    paquet_t pack;

    // fenetre du "Selective Repeat"
    etat_paquet_t send_paquets_state[SEQ_NUM_SIZE] = {ETAT_NONRECU};
    uint32_t borne_inf = 0;
    uint32_t window_len = SEQ_NUM_SIZE / 2;
    if (argc > 1)
        window_len = atoi(argv[1]);

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    int fin = 0; /* condition d'arrêt */
    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);


        if (check_integrity(&paquet)) {
            pack.type = ACK;

            if (dans_fenetre(borne_inf, paquet.num_seq, window_len) && send_paquets_state[paquet.num_seq] != ETAT_RECU) {
                send_paquets_state[paquet.num_seq] = ETAT_RECU;
                paquets[paquet.num_seq] = paquet;
            }
        } else {
            pack.type = NACK;
        }
        pack.num_seq = paquet.num_seq;
        vers_reseau(&pack);
        // petat(send_paquets_state);

        // decalage de la fenetre et restitution dans l'ordre
        int i = borne_inf;
        while (dans_fenetre(borne_inf, i, window_len)) {
            if (send_paquets_state[i] != ETAT_RECU)
                break;

            send_paquets_state[i] = ETAT_NONRECU;
            /* extraction des donnees du paquet recu */
            for (int j = 0; j < paquets[i].lg_info; j++) {
                message[j] = paquets[i].info[j];
            }

            /* remise des données à la couche application */
            fin = vers_application(message, paquets[i].lg_info);
            i = increment(i);
        }
        borne_inf = i;
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
