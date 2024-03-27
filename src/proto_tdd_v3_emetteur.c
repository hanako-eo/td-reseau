/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
* TRANSFERT DE DONNEES  v0                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */

    paquet_t paquets[SEQ_NUM_SIZE] = {0}; /* paquet utilisé par le protocole */
    paquet_t pack = {0};

    // fenetre du "Go-Back N"
    uint32_t write_cursor = 0;
    uint32_t send_cursor = 0;
    uint32_t borne_inf = 0;
    uint32_t window_len = 7;
    if (argc > 1)
        window_len = atoi(argv[1]);

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);
    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {
        /* construction des paquets */
        while (dans_fenetre(borne_inf, write_cursor, window_len) && taille_msg != 0) {
            paquet_t* paquet = &paquets[write_cursor];

            for (int i = 0; i < taille_msg; i++) {
                paquet->info[i] = message[i];
            }
            paquet->num_seq = write_cursor;
            paquet->lg_info = taille_msg;
            paquet->type = DATA;
            paquet->somme_ctrl = calcul_checksum(paquet);

            write_cursor = increment(write_cursor);
            /* lecture des donnees suivantes de la couche application */
            de_application(message, &taille_msg);
        }

        // Envoie des paquets
        while (dans_fenetre(borne_inf, send_cursor, window_len) && send_cursor != write_cursor) {
            if (borne_inf == send_cursor)
                depart_temporisateur(2000);
            
            vers_reseau(&paquets[send_cursor]);
            send_cursor = increment(send_cursor);
        }

        int event;
        while (borne_inf != send_cursor) {
            event = attendre();
            if (event != -1)
                break;

            de_reseau(&pack);
            if (pack.type == NACK)
                break;

            if (dans_fenetre(increment(borne_inf), pack.num_seq, window_len))
                borne_inf = pack.num_seq;
        }
        if (event == -1)
            arret_temporisateur();

        send_cursor = borne_inf;
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
