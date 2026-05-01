#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include "sqlite3.h"

sqlite3 *db;

void db_init() {
    _mkdir("output");

    int rc = sqlite3_open("output\\hireme.db", &db);
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

// ✓ Insérer un user et retourner son ID
int db_insert_user(const char *name) {
    char sql[256];
    char *err = 0;
    snprintf(sql, sizeof(sql), "INSERT INTO users (name) VALUES ('%s');", name);
    
    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Erreur insert user: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    
    // Récupérer l'ID de l'utilisateur inséré
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT last_insert_rowid();", -1, &stmt, 0);
    int user_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return user_id;
}

// ✓ Insérer une session
int db_insert_session(int user_id, const char *domain, const char *role, const char *level) {
    char sql[512];
    snprintf(sql, sizeof(sql), 
        "INSERT INTO sessions (user_id, domain, role, level, total_score, badge) VALUES (%d, '%s', '%s', '%s', 0, 'Pending');",
        user_id, domain, role, level);
    
    char *err = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Erreur insert session: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    
    // Récupérer l'ID de la session insérée
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT last_insert_rowid();", -1, &stmt, 0);
    int session_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        session_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return session_id;
}

// ✓ Récupérer une session par ID
char* db_get_session(int session_id) {
    static char result[1024];
    sqlite3_stmt *stmt;
    const char *sql = "SELECT user_id, domain, role, level, total_score FROM sessions WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return "{}";
    }
    sqlite3_bind_int(stmt, 1, session_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(result, sizeof(result), 
            "{\"session_id\":%d, \"user_id\":%d, \"domain\":\"%s\", \"role\":\"%s\", \"level\":\"%s\", \"score\":%d}",
            session_id,
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2),
            (const char*)sqlite3_column_text(stmt, 3),
            sqlite3_column_int(stmt, 4)
        );
    } else {
        snprintf(result, sizeof(result), "{}");
    }
    sqlite3_finalize(stmt);
    return result;
}