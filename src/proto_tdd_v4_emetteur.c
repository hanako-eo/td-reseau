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

bool completement_envoye(etat_paquet_t* send_paquets_state) {
    for (int i = 0; i < SEQ_NUM_SIZE; i++) {
        if (send_paquets_state[i] != ETAT_CONFIRME)
            return false;
    }
    return true;
}

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */

    etat_paquet_t send_paquets_state[SEQ_NUM_SIZE] = {ETAT_NONENVOYE};
    paquet_t paquets[SEQ_NUM_SIZE] = {0}; /* paquet utilisé par le protocole */
    paquet_t pack = {0};

    // fenetre du "Selective Repeat"
    uint32_t write_cursor = 0;
    uint32_t send_cursor = 0;
    uint32_t borne_inf = 0;
    uint32_t window_len = SEQ_NUM_SIZE / 2;
    if (argc > 1)
        window_len = atoi(argv[1]);

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);
    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 || !completement_envoye(send_paquets_state) ) {
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
            send_paquets_state[write_cursor] = ETAT_NONENVOYE;

            write_cursor = increment(write_cursor);
            /* lecture des donnees suivantes de la couche application */
            de_application(message, &taille_msg);
        }

        // // Envoie des paquets
        while (dans_fenetre(borne_inf, send_cursor, window_len) && send_cursor != write_cursor) {
            if (send_paquets_state[send_cursor] == ETAT_NONENVOYE) {
                depart_temporisateur_num(send_cursor + 1, 2000);

                send_paquets_state[send_cursor] = ETAT_ENVOYE;
                vers_reseau(&paquets[send_cursor]);
            }
            send_cursor = increment(send_cursor);
        }

        int event = attendre();
        if (event != -1) {
            send_paquets_state[event - 1] = ETAT_NONENVOYE;
        } else {
            de_reseau(&pack);

            if (send_paquets_state[pack.num_seq] == ETAT_ENVOYE) {
                arret_temporisateur_num(pack.num_seq + 1);

                if (pack.type == NACK) {
                    send_paquets_state[pack.num_seq] = ETAT_NONENVOYE;
                } else {
                    send_paquets_state[pack.num_seq] = ETAT_CONFIRME;
                }
            }
        }

        // decalage de la fenetre
        int i = borne_inf;
        while (dans_fenetre(borne_inf, i, window_len)) {
            if (send_paquets_state[i] != ETAT_CONFIRME)
                break;

            i = increment(i);
        }
        borne_inf = send_cursor = i;
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
