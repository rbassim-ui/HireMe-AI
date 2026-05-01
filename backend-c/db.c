#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

sqlite3 *db;

void db_init() {
    int rc = sqlite3_open("hireme.db", &db);
    if (rc != SQLITE_OK) {
        printf("Erreur ouverture DB: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    printf("Base de données ouverte avec succès.\n");
}

void db_create_tables() {
    char *err = 0;

    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER,"
        "domain TEXT,"
        "role TEXT,"
        "level TEXT,"
        "total_score REAL,"
        "badge TEXT,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS answers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_id INTEGER,"
        "question TEXT,"
        "answer TEXT,"
        "score INTEGER,"
        "feedback TEXT,"
        "FOREIGN KEY(session_id) REFERENCES sessions(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS scores ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_id INTEGER,"
        "average_score REAL,"
        "total_questions INTEGER,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(session_id) REFERENCES sessions(id)"
        ");";

    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Erreur création tables: %s\n", err);
        sqlite3_free(err);
    } else {
        printf("Tables créées avec succès.\n");
    }
}

void db_close() {
    sqlite3_close(db);
    printf("Base de données fermée.\n");
}