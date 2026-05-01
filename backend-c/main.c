#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

void db_init();
void db_create_tables();
void db_close();
void lancer_menu();

int main() {
    printf("=== HireMe AI - Interview Simulator ===\n");
    
    db_init();
    db_create_tables();
    lancer_menu();
    db_close();
    
    return 0;
}