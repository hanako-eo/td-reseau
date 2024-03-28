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
    unsigned char message[MAX_INFO]; // message de l'application
    int taille_msg; // taille du message

    etat_paquet_t etats_des_paquets[SEQ_NUM_SIZE] = {ETAT_NONENVOYE};
    paquet_t paquets[SEQ_NUM_SIZE]; // paquets d'envoie des messages
    paquet_t p_ack; // paquet de recuperation du ACK

    // fenetre du "Selective Repeat"
    uint32_t curseur_decriture = 0;
    uint32_t curseur_denvoie = 0;
    uint32_t borne_inf = 0;
    uint32_t taille_fenetre =  argc > 1 ? atoi(argv[1]) : SEQ_NUM_SIZE / 2;
    int8_t nombre_paquets_non_envoye = 0;

    init_reseau(EMISSION);

    printf("[TRP] Taille de la fenetre d'envoie : %d.\n", taille_fenetre);
    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);

    // systeme d'envoie des paquets via double curseur
    while (taille_msg != 0 || nombre_paquets_non_envoye != 0) {
        // construction des paquets
        while (dans_fenetre(borne_inf, curseur_decriture, taille_fenetre) && taille_msg != 0) {
            paquet_t* paquet = &paquets[curseur_decriture];

            for (int i = 0; i < taille_msg; i++) {
                paquet->info[i] = message[i];
            }
            paquet->num_seq = curseur_decriture;
            paquet->lg_info = taille_msg;
            paquet->type = DATA;
            paquet->somme_ctrl = calcul_somme_ctrl(paquet);
            etats_des_paquets[curseur_decriture] = ETAT_NONENVOYE;

            curseur_decriture = increment(curseur_decriture);
            nombre_paquets_non_envoye++;

            // lecture des donnees suivantes de la couche application
            de_application(message, &taille_msg);
        }

        // envoie des paquets
        while (
            dans_fenetre(borne_inf, curseur_denvoie, taille_fenetre) &&
            curseur_denvoie != curseur_decriture
        ) {
            if (etats_des_paquets[curseur_denvoie] == ETAT_NONENVOYE) {
                depart_temporisateur_num(curseur_denvoie + 1, 2000);

                etats_des_paquets[curseur_denvoie] = ETAT_ENVOYE;
                vers_reseau(&paquets[curseur_denvoie]);
            }
            curseur_denvoie = increment(curseur_denvoie);
        }

        // attente d'un paquet ou d'un timeout
        int event = attendre();
        if (event == -1) {
            de_reseau(&p_ack);

            // pour evite 
            if (etats_des_paquets[p_ack.num_seq] != ETAT_ENVOYE)
                goto decalage;

            arret_temporisateur_num(p_ack.num_seq + 1);

            if (p_ack.type == ACK) {
                etats_des_paquets[p_ack.num_seq] = ETAT_CONFIRME;
                nombre_paquets_non_envoye--;
            } else etats_des_paquets[p_ack.num_seq] = ETAT_NONENVOYE;
        } else etats_des_paquets[event - 1] = ETAT_NONENVOYE;

    decalage:
        // decalage de la fenetre
        int i = borne_inf;
        while (dans_fenetre(borne_inf, i, taille_fenetre)) {
            if (etats_des_paquets[i] != ETAT_CONFIRME)
                break;

            i = increment(i);
        }
        borne_inf = curseur_denvoie = i;
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
