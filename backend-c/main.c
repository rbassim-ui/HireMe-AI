#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "sqlite3.h"

void db_init();
void db_create_tables();
void db_close();
void lancer_menu();
void start_api_server();

// Thread pour le serveur API
DWORD WINAPI api_thread_func(LPVOID param) {
    (void)param;
    start_api_server();
    return 0;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    printf("=== HireMe AI - Interview Simulator ===\n");
    
    db_init();
    db_create_tables();
    
    // Démarrer le serveur API dans un thread
    HANDLE api_handle = CreateThread(NULL, 0, api_thread_func, NULL, 0, NULL);
    if (api_handle == NULL) {
        printf("Erreur création du thread API\n");
    }

    // En mode non interactif, garder uniquement l'API vivante.
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD console_mode = 0;
    if (stdin_handle == INVALID_HANDLE_VALUE || !GetConsoleMode(stdin_handle, &console_mode)) {
        printf("Mode non interactif détecté: menu CLI ignoré, API active.\n");
        WaitForSingleObject(api_handle, INFINITE);
    } else {
        // Lancer le menu dans le thread principal
        lancer_menu();
    }
    
    db_close();
    
    return 0;
}