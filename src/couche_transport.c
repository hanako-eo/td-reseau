#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "couche_transport.h"
#include "services_reseau.h"
#include "application.h"

/* ************************************************************************** */
/* *************** Fonctions utilitaires couche transport ******************* */
/* ************************************************************************** */

// RAJOUTER VOS FONCTIONS DANS CE FICHIER...

int calcul_somme_ctrl(paquet_t* p) {
    int c = p->type ^ p->num_seq ^ p->lg_info;
    for (size_t i = 0; i < p->lg_info; i++)
        c ^= p->info[i];

    return c;
}

bool controle_integrite(paquet_t* p) {
    return calcul_somme_ctrl(p) == p->somme_ctrl;
}


/* ===================== Fenêtre d'anticipation ============================= */

/*--------------------------------------*/
/* Fonction d'inclusion dans la fenetre */
/*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille) {

    unsigned int sup = (inf+taille-1) % SEQ_NUM_SIZE;

    return
        /* inf <= pointeur <= sup */
        ( inf <= sup && pointeur >= inf && pointeur <= sup ) ||
        /* sup < inf <= pointeur */
        ( sup < inf && pointeur >= inf) ||
        /* pointeur <= sup < inf */
        ( sup < inf && pointeur <= sup);
}

unsigned int increment(unsigned int n) {
    return (n + 1) % SEQ_NUM_SIZE;
}
