#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

// Déclarations des fonctions de db.c
void db_init();
void db_create_tables();
void db_close();

int main() {
    printf("=== HireMe AI - Interview Simulator ===\n\n");
    
    db_init();
    db_create_tables();
    db_close();
    
    return 0;
}