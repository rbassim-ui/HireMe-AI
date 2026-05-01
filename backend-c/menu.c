#include <stdio.h>
#include <stdlib.h>

void afficher_menu() {
    printf("\n");
    printf("=====================================\n");
    printf("       HireMe AI - Interview         \n");
    printf("=====================================\n");
    printf("  1. Start Interview                 \n");
    printf("  2. View History                    \n");
    printf("  3. Exit                            \n");
    printf("=====================================\n");
    printf("Votre choix: ");
}

void lancer_menu() {
    int choix;

    while (1) {
        afficher_menu();
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                printf("\n>> Lancement de l'entretien...\n");
                // TODO: appeler start_interview()
                break;
            case 2:
                printf("\n>> Affichage de l'historique...\n");
                // TODO: appeler view_history()
                break;
            case 3:
                printf("\nAu revoir ! Bonne chance dans vos entretiens.\n");
                exit(0);
            default:
                printf("\nChoix invalide. Entrez 1, 2 ou 3.\n");
        }
    }
}