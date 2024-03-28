/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
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
#include "config.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; // message de l'application
    int taille_msg; // taille du message

    paquet_t paquets[SEQ_NUM_SIZE]; // paquets d'envoie des messages
    paquet_t p_ack; // paquet de recuperation du ACK

    // fenetre du "Go-Back N"
    uint32_t curseur_decriture = 0;
    uint32_t curseur_denvoie = 0;
    uint32_t borne_inf = 0;
    size_t taille_fenetre = argc > 1 ? atoi(argv[1]) : 7;

    init_reseau(EMISSION);

    printf("[TRP] Taille de la fenetre d'envoie : %d.\n", taille_fenetre);
    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    de_application(message, &taille_msg);

    // systeme d'envoie des paquets via double curseur
    while (taille_msg != 0) {
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

            curseur_decriture = increment(curseur_decriture);

            // lecture des donnees suivantes de la couche application
            de_application(message, &taille_msg);
        }

        // envoie des paquets
        while (dans_fenetre(borne_inf, curseur_denvoie, taille_fenetre) && curseur_denvoie != curseur_decriture) {
            if (borne_inf == curseur_denvoie)
                depart_temporisateur(recuperation_temps_attente());
            
            vers_reseau(&paquets[curseur_denvoie]);
            curseur_denvoie = increment(curseur_denvoie);
        }

        // reception des evenements
        int evenement;
        while (borne_inf != curseur_denvoie) {
            evenement = attendre();
            // quitte pour forcer le renvoi du paquets
            if (evenement != -1)
                break;

            de_reseau(&p_ack);
            // quitte pour forcer le renvoi du paquets
            if (p_ack.type == NACK)
                break;

            if (dans_fenetre(increment(borne_inf), p_ack.num_seq, taille_fenetre))
                borne_inf = p_ack.num_seq;
        }
        if (evenement == -1)
            arret_temporisateur();

        curseur_denvoie = borne_inf;
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
