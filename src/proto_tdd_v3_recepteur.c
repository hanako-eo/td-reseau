/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v0                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet; /* paquet utilisé par le protocole */
    paquet_t pack;

    // fenetre du "Go-Back N"
    uint8_t next_cursor = 0;

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    int fin = 0; /* condition d'arrêt */
    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);

        if (check_integrity(&paquet) && dans_fenetre(next_cursor, paquet.num_seq, 1)) {
            next_cursor = (paquet.num_seq + 1) % SEQ_NUM_SIZE;
            /* extraction des donnees du paquet recu */
            for (int i=0; i<paquet.lg_info; i++) {
                message[i] = paquet.info[i];
            }
            pack.type = ACK;

            /* remise des données à la couche application */
            fin = vers_application(message, paquet.lg_info);
        } else {
            pack.type = NACK;
        }
        // numero du prochain index du paquet a envoyer
        pack.num_seq = next_cursor;

        vers_reseau(&pack);
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
