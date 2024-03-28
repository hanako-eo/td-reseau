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

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; // message pour l'application
    paquet_t paquets_buffer[SEQ_NUM_SIZE]; // buffer des paquets recus
    paquet_t paquet; // paquet d'envoie du message
    paquet_t p_ack; // paquet de recuperation du ACK

    // fenetre du "Selective Repeat"
    etat_paquet_t etats_des_paquets[SEQ_NUM_SIZE] = {ETAT_NONRECU};
    uint32_t borne_inf = 0;
    size_t taille_fenetre =  argc > 1 ? atoi(argv[1]) : SEQ_NUM_SIZE / 2;

    init_reseau(RECEPTION);

    printf("[TRP] Taille de la fenetre d'envoie : %ld.\n", taille_fenetre);
    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    int fin = 0; // condition d'arrêt
    // tant que le récepteur reçoit des données
    while (!fin) {
        de_reseau(&paquet);

        if (controle_integrite(&paquet)) {
            p_ack.type = ACK;

            if (dans_fenetre(borne_inf, paquet.num_seq, taille_fenetre) && etats_des_paquets[paquet.num_seq] != ETAT_RECU) {
                etats_des_paquets[paquet.num_seq] = ETAT_RECU;
                paquets_buffer[paquet.num_seq] = paquet;
            }
        } else p_ack.type = NACK;
        p_ack.num_seq = paquet.num_seq;
        vers_reseau(&p_ack);

        // decalage de la fenetre et restitution dans l'ordre
        int i = borne_inf;
        while (dans_fenetre(borne_inf, i, taille_fenetre)) {
            if (etats_des_paquets[i] != ETAT_RECU)
                break;

            etats_des_paquets[i] = ETAT_NONRECU;
            // extraction des donnees du paquet recu
            for (int j = 0; j < paquets_buffer[i].lg_info; j++) {
                message[j] = paquets_buffer[i].info[j];
            }

            // remise des données à la couche application
            fin = vers_application(message, paquets_buffer[i].lg_info);
            i = increment(i);
        }
        borne_inf = i;
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
