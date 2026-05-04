#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include "sqlite3.h"

sqlite3 *db;

static void json_escape(const char *src, char *dst, size_t dst_size) {
    size_t i = 0;
    if (!dst || dst_size == 0) return;
    if (!src) src = "";

    for (size_t j = 0; src[j] != '\0' && i + 1 < dst_size; j++) {
        unsigned char c = (unsigned char)src[j];
        if (c == '"' || c == '\\') {
            if (i + 2 >= dst_size) break;
            dst[i++] = '\\';
            dst[i++] = (char)c;
        } else if (c == '\n') {
            if (i + 2 >= dst_size) break;
            dst[i++] = '\\';
            dst[i++] = 'n';
        } else if (c == '\r') {
            if (i + 2 >= dst_size) break;
            dst[i++] = '\\';
            dst[i++] = 'r';
        } else if (c == '\t') {
            if (i + 2 >= dst_size) break;
            dst[i++] = '\\';
            dst[i++] = 't';
        } else {
            dst[i++] = (char)c;
        }
    }
    dst[i] = '\0';
}

static const char *safe_text(const unsigned char *value) {
    return value ? (const char *)value : "";
}

static int column_exists(const char *table_name, const char *column_name) {
    char sql[128];
    snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", table_name);

    sqlite3_stmt *stmt = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        return 0;
    }

    int exists = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (name && strcmp((const char *)name, column_name) == 0) {
            exists = 1;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return exists;
}

static void ensure_user_columns(void) {
    char *err = 0;
    if (!column_exists("users", "password")) {
        sqlite3_exec(db, "ALTER TABLE users ADD COLUMN password TEXT DEFAULT '';", 0, 0, &err);
        if (err) {
            printf("Erreur ajout colonne password: %s\n", err);
            sqlite3_free(err);
            err = 0;
        }
    }

    if (!column_exists("users", "email")) {
        sqlite3_exec(db, "ALTER TABLE users ADD COLUMN email TEXT DEFAULT '';", 0, 0, &err);
        if (err) {
            printf("Erreur ajout colonne email: %s\n", err);
            sqlite3_free(err);
        }
    }
}

void db_init() {
    _mkdir("output");
    int rc = sqlite3_open("output\\hireme.db", &db);
    if (rc != SQLITE_OK) {
        printf("Erreur ouverture DB: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);
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
        "total_score REAL DEFAULT 0,"
        "badge TEXT DEFAULT 'Pending',"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(user_id) REFERENCES users(id)"
        ");"

        "CREATE TABLE IF NOT EXISTS answers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_id INTEGER,"
        "question TEXT,"
        "answer TEXT,"
        "score INTEGER DEFAULT 0,"
        "feedback TEXT,"
        "created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
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
        ensure_user_columns();
    }
}

void db_close() {
    sqlite3_close(db);
    printf("Base de données fermée.\n");
}

// ✓ Insérer un user
int db_insert_user(const char *name, const char *password) {
    const char *sql = "INSERT INTO users (name, password) VALUES (?, ?);";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare insert user: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name ? name : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password ? password : "", -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("Erreur insert user: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return (int)sqlite3_last_insert_rowid(db);
}

// ✓ Insérer une session
int db_insert_session(int user_id, const char *domain, const char *role, const char *level) {
    const char *sql = "INSERT INTO sessions (user_id, domain, role, level) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare insert session: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, domain ? domain : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role ? role : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, level ? level : "", -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("Erreur insert session: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return (int)sqlite3_last_insert_rowid(db);
}

// ✓ Insérer une réponse pour une session
int db_insert_answer(int session_id, const char *question, const char *answer, int score, const char *feedback) {
    const char *sql =
        "INSERT INTO answers (session_id, question, answer, score, feedback) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = 0;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("Erreur prepare insert answer: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_int(stmt, 1, session_id);
    sqlite3_bind_text(stmt, 2, question ? question : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, answer ? answer : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, score);
    sqlite3_bind_text(stmt, 5, feedback ? feedback : "", -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        printf("Erreur insert answer: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

// ✓ Mettre à jour le score final d'une session
int db_update_session_score(int session_id, float total_score, const char *badge) {
    const char *sql = "UPDATE sessions SET total_score=?, badge=? WHERE id=?;";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare update score: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_double(stmt, 1, total_score);
    sqlite3_bind_text(stmt, 2, badge ? badge : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, session_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("Erreur update session score: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    int total_questions = 0;
    const char *count_sql = "SELECT COUNT(*) FROM answers WHERE session_id = ?;";
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, session_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_questions = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    sqlite3_exec(db, "DELETE FROM scores WHERE session_id = ?;", 0, 0, 0);
    const char *delete_sql = "DELETE FROM scores WHERE session_id = ?;";
    if (sqlite3_prepare_v2(db, delete_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, session_id);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    const char *insert_sql =
        "INSERT INTO scores (session_id, average_score, total_questions) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, session_id);
        sqlite3_bind_double(stmt, 2, total_score);
        sqlite3_bind_int(stmt, 3, total_questions);
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        printf("Erreur insert score summary: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

int db_find_user_by_credentials(const char *name, const char *password) {
    const char *sql =
        "SELECT id FROM users WHERE name = ? AND COALESCE(password, '') = COALESCE(?, '') LIMIT 1;";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare login: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name ? name : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password ? password : "", -1, SQLITE_TRANSIENT);

    int user_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return user_id;
}

int db_user_name_exists(const char *name) {
    const char *sql = "SELECT id FROM users WHERE name = ? LIMIT 1;";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare user exists: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, name ? name : "", -1, SQLITE_TRANSIENT);
    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

int db_update_user_name(int user_id, const char *name) {
    const char *sql = "UPDATE users SET name = ? WHERE id = ?;";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare update user name: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, name ? name : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_update_user_password(int user_id, const char *password) {
    const char *sql = "UPDATE users SET password = ? WHERE id = ?;";
    sqlite3_stmt *stmt = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        printf("Erreur prepare update password: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, password ? password : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int db_delete_user_account(int user_id) {
    const char *queries[] = {
        "DELETE FROM answers WHERE session_id IN (SELECT id FROM sessions WHERE user_id = ?);",
        "DELETE FROM scores WHERE session_id IN (SELECT id FROM sessions WHERE user_id = ?);",
        "DELETE FROM sessions WHERE user_id = ?;",
        "DELETE FROM users WHERE id = ?;"
    };

    if (sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0) != SQLITE_OK) {
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        sqlite3_stmt *stmt = 0;
        if (sqlite3_prepare_v2(db, queries[i], -1, &stmt, 0) != SQLITE_OK) {
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            return 0;
        }
        sqlite3_bind_int(stmt, 1, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) {
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            return 0;
        }
    }

    if (sqlite3_exec(db, "COMMIT;", 0, 0, 0) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        return 0;
    }

    return 1;
}

char* db_get_user_account(int user_id) {
    static char result[12288];
    memset(result, 0, sizeof(result));

    sqlite3_stmt *stmt = 0;
    const char *user_sql =
        "SELECT id, name, COALESCE(email, ''), created_at FROM users WHERE id = ?;";

    int found = 0;
    char user_name[256] = "";
    char user_email[256] = "";
    char user_created_at[64] = "";

    if (sqlite3_prepare_v2(db, user_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
            json_escape(safe_text(sqlite3_column_text(stmt, 1)), user_name, sizeof(user_name));
            json_escape(safe_text(sqlite3_column_text(stmt, 2)), user_email, sizeof(user_email));
            json_escape(safe_text(sqlite3_column_text(stmt, 3)), user_created_at, sizeof(user_created_at));
        }
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    if (!found) {
        snprintf(result, sizeof(result), "{\"success\":false,\"message\":\"User not found\"}");
        return result;
    }

    const char *stats_sql =
        "SELECT COUNT(*), AVG(total_score), MAX(total_score) FROM sessions WHERE user_id = ? AND total_score > 0;";
    int total_sessions = 0;
    double avg_score = 0.0;
    double best_score = 0.0;

    if (sqlite3_prepare_v2(db, stats_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_sessions = sqlite3_column_int(stmt, 0);
            avg_score = sqlite3_column_double(stmt, 1);
            best_score = sqlite3_column_double(stmt, 2);
        }
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    const char *sessions_sql =
        "SELECT id, domain, role, level, total_score, badge, created_at "
        "FROM sessions WHERE user_id = ? ORDER BY created_at DESC LIMIT 12;";

    char sessions_json[4096];
    memset(sessions_json, 0, sizeof(sessions_json));
    strcat(sessions_json, "[");
    int first = 1;

    if (sqlite3_prepare_v2(db, sessions_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            char domain[128] = "";
            char role[128] = "";
            char level[64] = "";
            char badge[128] = "";
            char created_at[64] = "";
            char item[1024];

            json_escape(safe_text(sqlite3_column_text(stmt, 1)), domain, sizeof(domain));
            json_escape(safe_text(sqlite3_column_text(stmt, 2)), role, sizeof(role));
            json_escape(safe_text(sqlite3_column_text(stmt, 3)), level, sizeof(level));
            json_escape(safe_text(sqlite3_column_text(stmt, 5)), badge, sizeof(badge));
            json_escape(safe_text(sqlite3_column_text(stmt, 6)), created_at, sizeof(created_at));

            if (!first) strcat(sessions_json, ",");
            snprintf(item, sizeof(item),
                "{\"id\":%d,\"domain\":\"%s\",\"role\":\"%s\",\"level\":\"%s\",\"score\":%.1f,\"badge\":\"%s\",\"date\":\"%s\"}",
                sqlite3_column_int(stmt, 0), domain, role, level, sqlite3_column_double(stmt, 4), badge, created_at);
            strcat(sessions_json, item);
            first = 0;
        }
    }
    sqlite3_finalize(stmt);
    strcat(sessions_json, "]");

    snprintf(result, sizeof(result),
        "{\"success\":true,\"user\":{\"id\":%d,\"name\":\"%s\",\"email\":\"%s\",\"created_at\":\"%s\"},"
        "\"stats\":{\"total_sessions\":%d,\"avg_score\":%.1f,\"best_score\":%.1f},"
        "\"sessions\":%s}",
        user_id, user_name, user_email, user_created_at, total_sessions, avg_score, best_score, sessions_json);

    return result;
}

char* db_get_user_history(int user_id, const char *domain_filter, const char *search_filter, int limit) {
    static char result[16384];
    memset(result, 0, sizeof(result));

    if (limit <= 0 || limit > 100) {
        limit = 50;
    }

    char domain_pattern[128] = "%";
    char search_pattern[128] = "%";

    if (domain_filter && domain_filter[0]) {
        size_t i = 0;
        domain_pattern[i++] = '%';
        for (const char *p = domain_filter; *p && i + 2 < sizeof(domain_pattern); p++) {
            domain_pattern[i++] = (char)tolower((unsigned char)*p);
        }
        domain_pattern[i++] = '%';
        domain_pattern[i] = '\0';
    }

    if (search_filter && search_filter[0]) {
        size_t i = 0;
        search_pattern[i++] = '%';
        for (const char *p = search_filter; *p && i + 2 < sizeof(search_pattern); p++) {
            search_pattern[i++] = (char)tolower((unsigned char)*p);
        }
        search_pattern[i++] = '%';
        search_pattern[i] = '\0';
    }

    const char *stats_sql =
        "SELECT COUNT(*), COALESCE(AVG(total_score), 0), COALESCE(MAX(total_score), 0) "
        "FROM sessions "
        "WHERE user_id = ? AND total_score > 0 "
        "AND (? = '' OR lower(domain) LIKE ? ) "
        "AND (? = '' OR (lower(domain) LIKE ? OR lower(role) LIKE ? OR lower(level) LIKE ? OR lower(badge) LIKE ?));";

    sqlite3_stmt *stmt = 0;
    int total_sessions = 0;
    double avg_score = 0.0;
    double best_score = 0.0;

    if (sqlite3_prepare_v2(db, stats_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, domain_filter ? domain_filter : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, domain_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, search_filter ? search_filter : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, search_pattern, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_sessions = sqlite3_column_int(stmt, 0);
            avg_score = sqlite3_column_double(stmt, 1);
            best_score = sqlite3_column_double(stmt, 2);
        }
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    const char *items_sql =
        "SELECT id, domain, role, level, total_score, badge, created_at "
        "FROM sessions "
        "WHERE user_id = ? AND total_score > 0 "
        "AND (? = '' OR lower(domain) LIKE ? ) "
        "AND (? = '' OR (lower(domain) LIKE ? OR lower(role) LIKE ? OR lower(level) LIKE ? OR lower(badge) LIKE ?)) "
        "ORDER BY created_at DESC LIMIT ?;";

    char items_json[12288];
    memset(items_json, 0, sizeof(items_json));
    strcat(items_json, "[");
    int first = 1;

    if (sqlite3_prepare_v2(db, items_sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_text(stmt, 2, domain_filter ? domain_filter : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, domain_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, search_filter ? search_filter : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, search_pattern, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 9, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            char domain[128] = "";
            char role[128] = "";
            char level[64] = "";
            char badge[128] = "";
            char created_at[64] = "";
            char item[1024];

            json_escape(safe_text(sqlite3_column_text(stmt, 1)), domain, sizeof(domain));
            json_escape(safe_text(sqlite3_column_text(stmt, 2)), role, sizeof(role));
            json_escape(safe_text(sqlite3_column_text(stmt, 3)), level, sizeof(level));
            json_escape(safe_text(sqlite3_column_text(stmt, 5)), badge, sizeof(badge));
            json_escape(safe_text(sqlite3_column_text(stmt, 6)), created_at, sizeof(created_at));

            if (!first) strcat(items_json, ",");
            snprintf(item, sizeof(item),
                "{\"id\":%d,\"domain\":\"%s\",\"role\":\"%s\",\"level\":\"%s\",\"score\":%.1f,\"badge\":\"%s\",\"date\":\"%s\"}",
                sqlite3_column_int(stmt, 0), domain, role, level, sqlite3_column_double(stmt, 4), badge, created_at);
            strcat(items_json, item);
            first = 0;
        }
    }
    sqlite3_finalize(stmt);
    strcat(items_json, "]");

    snprintf(result, sizeof(result),
        "{\"success\":true,\"total_sessions\":%d,\"avg_score\":%.1f,\"best_score\":%.1f,\"items\":%s}",
        total_sessions, avg_score, best_score, items_json);

    return result;
}

// ✓ Récupérer une session
char* db_get_session(int session_id) {
    static char result[1024];
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT s.user_id, s.domain, s.role, s.level, s.total_score, s.badge, u.name "
        "FROM sessions s JOIN users u ON s.user_id = u.id WHERE s.id = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) return "{}";

    sqlite3_bind_int(stmt, 1, session_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(result, sizeof(result),
            "{\"session_id\":%d,\"user_id\":%d,\"name\":\"%s\","
            "\"domain\":\"%s\",\"role\":\"%s\",\"level\":\"%s\","
            "\"total_score\":%.1f,\"badge\":\"%s\"}",
            session_id,
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 6),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2),
            (const char*)sqlite3_column_text(stmt, 3),
            sqlite3_column_double(stmt, 4),
            (const char*)sqlite3_column_text(stmt, 5)
        );
    } else {
        snprintf(result, sizeof(result), "{}");
    }
    sqlite3_finalize(stmt);
    return result;
}

// ✓ Récupérer toutes les sessions (historique)
char* db_get_all_sessions() {
    static char result[8192];
    memset(result, 0, sizeof(result));

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT s.id, u.name, s.domain, s.role, s.level, s.total_score, s.badge, s.created_at "
        "FROM sessions s JOIN users u ON s.user_id = u.id "
        "ORDER BY s.created_at DESC LIMIT 20;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) return "[]";

    strcat(result, "[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char entry[1024];
        if (!first) strcat(result, ",");
        snprintf(entry, sizeof(entry),
            "{\"id\":%d,\"name\":\"%s\",\"domain\":\"%s\","
            "\"role\":\"%s\",\"level\":\"%s\","
            "\"score\":%.1f,\"badge\":\"%s\",\"date\":\"%s\"}",
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2),
            (const char*)sqlite3_column_text(stmt, 3),
            (const char*)sqlite3_column_text(stmt, 4),
            sqlite3_column_double(stmt, 5),
            (const char*)sqlite3_column_text(stmt, 6),
            (const char*)sqlite3_column_text(stmt, 7)
        );
        strcat(result, entry);
        first = 0;
    }
    strcat(result, "]");
    sqlite3_finalize(stmt);
    return result;
}

// ✓ Calculer et sauvegarder le score moyen d'une session
float db_calculate_average_score(int session_id) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT AVG(score), COUNT(*) FROM answers WHERE session_id = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) return 0.0;

    sqlite3_bind_int(stmt, 1, session_id);

    float avg = 0.0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avg   = (float)sqlite3_column_double(stmt, 0);
        count = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    // Sauvegarder dans la table scores
    char sql2[256];
    snprintf(sql2, sizeof(sql2),
        "INSERT INTO scores (session_id, average_score, total_questions) "
        "VALUES (%d, %.2f, %d);",
        session_id, avg, count);

    char *err = 0;
    sqlite3_exec(db, sql2, 0, 0, &err);
    if (err) sqlite3_free(err);

    return avg;
}

// ✓ Récupérer les stats globales
char* db_get_stats() {
    static char result[512];
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT COUNT(*) as total_sessions, "
        "AVG(total_score) as avg_score, "
        "MAX(total_score) as best_score "
        "FROM sessions WHERE total_score > 0;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) return "{}";

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(result, sizeof(result),
            "{\"total_sessions\":%d,\"avg_score\":%.1f,\"best_score\":%.1f}",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_double(stmt, 1),
            sqlite3_column_double(stmt, 2)
        );
    } else {
        snprintf(result, sizeof(result), "{}");
    }
    sqlite3_finalize(stmt);
    return result;
}

// ✓ Stats enrichies pour le dashboard
char* db_get_dashboard_stats() {
    static char result[4096];
    memset(result, 0, sizeof(result));

    sqlite3_stmt *stmt = 0;
    const char *global_sql =
        "SELECT COUNT(*) as total_sessions, "
        "AVG(total_score) as avg_score, "
        "MAX(total_score) as best_score "
        "FROM sessions WHERE total_score > 0;";

    int total_sessions = 0;
    double avg_score = 0.0;
    double best_score = 0.0;

    if (sqlite3_prepare_v2(db, global_sql, -1, &stmt, 0) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        total_sessions = sqlite3_column_int(stmt, 0);
        avg_score = sqlite3_column_double(stmt, 1);
        best_score = sqlite3_column_double(stmt, 2);
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    char best_domain[128] = "";
    double best_domain_avg = 0.0;
    int best_domain_sessions = 0;

    const char *best_domain_sql =
        "SELECT domain, AVG(total_score) as avg_score, COUNT(*) as sessions "
        "FROM sessions WHERE total_score > 0 AND domain IS NOT NULL AND domain != '' "
        "GROUP BY domain "
        "ORDER BY avg_score DESC, sessions DESC "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(db, best_domain_sql, -1, &stmt, 0) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *domain_text = sqlite3_column_text(stmt, 0);
        if (domain_text) {
            strncpy(best_domain, (const char *)domain_text, sizeof(best_domain) - 1);
            best_domain[sizeof(best_domain) - 1] = '\0';
        }
        best_domain_avg = sqlite3_column_double(stmt, 1);
        best_domain_sessions = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
    stmt = 0;

    const char *domains_sql =
        "SELECT domain, AVG(total_score) as avg_score, COUNT(*) as sessions, MAX(total_score) as best_score "
        "FROM sessions WHERE total_score > 0 AND domain IS NOT NULL AND domain != '' "
        "GROUP BY domain "
        "ORDER BY avg_score DESC, sessions DESC "
        "LIMIT 5;";

    char domains_json[2048];
    memset(domains_json, 0, sizeof(domains_json));
    strcat(domains_json, "[");
    int first = 1;

    if (sqlite3_prepare_v2(db, domains_sql, -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            char item[256];
            const unsigned char *domain_text = sqlite3_column_text(stmt, 0);
            const char *domain_name = domain_text ? (const char *)domain_text : "";
            double domain_avg = sqlite3_column_double(stmt, 1);
            int domain_sessions = sqlite3_column_int(stmt, 2);
            double domain_best = sqlite3_column_double(stmt, 3);

            if (!first) strcat(domains_json, ",");
            snprintf(item, sizeof(item),
                "{\"domain\":\"%s\",\"avg_score\":%.1f,\"sessions\":%d,\"best_score\":%.1f}",
                domain_name, domain_avg, domain_sessions, domain_best);
            strcat(domains_json, item);
            first = 0;
        }
    }
    sqlite3_finalize(stmt);
    strcat(domains_json, "]");

    snprintf(result, sizeof(result),
        "{\"total_sessions\":%d,\"avg_score\":%.1f,\"best_score\":%.1f,\"best_domain\":\"%s\",\"best_domain_avg\":%.1f,\"best_domain_sessions\":%d,\"domains\":%s}",
        total_sessions, avg_score, best_score, best_domain, best_domain_avg, best_domain_sessions, domains_json);

    return result;
}