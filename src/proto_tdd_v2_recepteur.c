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
    unsigned char message[MAX_INFO]; // message pour l'application
    paquet_t paquet; // paquet d'envoie du message
    paquet_t p_ack; // paquet de recuperation du ACK
    int fin = 0; // condition d'arrêt

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    // tant que le récepteur reçoit des données
    while (!fin) {
        de_reseau(&paquet);

        if (controle_integrite(&paquet)) {
            // extraction des donnees du paquet recu
            for (int i=0; i<paquet.lg_info; i++) {
                message[i] = paquet.info[i];
            }
            p_ack.type = ACK;

            // suppretion des paquets doublons 
            if (p_ack.num_seq != paquet.num_seq)
                // remise des données à la couche application
                fin = vers_application(message, paquet.lg_info);
        } else p_ack.type = NACK;
        p_ack.num_seq = paquet.num_seq;

        vers_reseau(&p_ack);
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
