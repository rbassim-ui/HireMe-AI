/**
 * test_db.c - Tests automatiques SQLite
 * Jour 22 : Tests automatiques
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../sqlite3.h"

// Couleurs pour le terminal
#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

// Compteurs
int tests_passed = 0;
int tests_failed = 0;

// Macro de test
#define ASSERT(condition, message) do { \
    if (condition) { \
        printf(GREEN "  ✓ PASS" RESET " — %s\n", message); \
        tests_passed++; \
    } else { \
        printf(RED "  ✗ FAIL" RESET " — %s\n", message); \
        tests_failed++; \
    } \
} while(0)

// Base de données de test
sqlite3 *db;

void setup_test_db() {
    sqlite3_open(":memory:", &db);
    const char *sql =
        "CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, created_at TEXT DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE sessions (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, domain TEXT, role TEXT, level TEXT, total_score REAL DEFAULT 0, badge TEXT DEFAULT 'Pending', created_at TEXT DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE answers (id INTEGER PRIMARY KEY AUTOINCREMENT, session_id INTEGER, question TEXT, answer TEXT, score INTEGER DEFAULT 0, feedback TEXT, created_at TEXT DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE scores (id INTEGER PRIMARY KEY AUTOINCREMENT, session_id INTEGER, average_score REAL, total_questions INTEGER, created_at TEXT DEFAULT CURRENT_TIMESTAMP);";
    sqlite3_exec(db, sql, 0, 0, 0);
}

void teardown_test_db() {
    sqlite3_close(db);
}

// ── Test 1 : Insertion user ──
void test_insert_user() {
    printf("\n" YELLOW "► Test: Insertion utilisateur" RESET "\n");

    const char *sql = "INSERT INTO users (name) VALUES ('TestUser');";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    ASSERT(rc == SQLITE_OK, "Insertion user réussie");

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT name FROM users WHERE name='TestUser';", -1, &stmt, 0);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    ASSERT(found, "User trouvé dans la DB");
    if (found) {
        const char *name = (const char*)sqlite3_column_text(stmt, 0);
        ASSERT(strcmp(name, "TestUser") == 0, "Nom correct");
    }
    sqlite3_finalize(stmt);
}

// ── Test 2 : Insertion session ──
void test_insert_session() {
    printf("\n" YELLOW "► Test: Insertion session" RESET "\n");

    const char *sql = "INSERT INTO sessions (user_id, domain, role, level) VALUES (1, 'TECH', 'Developer', 'Beginner');";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    ASSERT(rc == SQLITE_OK, "Insertion session réussie");

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT domain, role, level FROM sessions WHERE user_id=1;", -1, &stmt, 0);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    ASSERT(found, "Session trouvée dans la DB");
    if (found) {
        ASSERT(strcmp((const char*)sqlite3_column_text(stmt, 0), "TECH") == 0, "Domaine correct");
        ASSERT(strcmp((const char*)sqlite3_column_text(stmt, 1), "Developer") == 0, "Rôle correct");
        ASSERT(strcmp((const char*)sqlite3_column_text(stmt, 2), "Beginner") == 0, "Niveau correct");
    }
    sqlite3_finalize(stmt);
}

// ── Test 3 : Insertion answer ──
void test_insert_answer() {
    printf("\n" YELLOW "► Test: Insertion réponse" RESET "\n");

    const char *sql = "INSERT INTO answers (session_id, question, answer, score, feedback) VALUES (1, 'Question test?', 'Réponse test', 8, 'Bon travail');";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    ASSERT(rc == SQLITE_OK, "Insertion réponse réussie");

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT score, feedback FROM answers WHERE session_id=1;", -1, &stmt, 0);
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    ASSERT(found, "Réponse trouvée dans la DB");
    if (found) {
        ASSERT(sqlite3_column_int(stmt, 0) == 8, "Score correct (8)");
        ASSERT(strcmp((const char*)sqlite3_column_text(stmt, 1), "Bon travail") == 0, "Feedback correct");
    }
    sqlite3_finalize(stmt);
}

// ── Test 4 : Calcul score moyen ──
void test_average_score() {
    printf("\n" YELLOW "► Test: Calcul score moyen" RESET "\n");

    // Ajouter plusieurs réponses
    sqlite3_exec(db, "INSERT INTO answers (session_id, question, answer, score, feedback) VALUES (1, 'Q2?', 'R2', 6, 'OK');", 0, 0, 0);
    sqlite3_exec(db, "INSERT INTO answers (session_id, question, answer, score, feedback) VALUES (1, 'Q3?', 'R3', 10, 'Excellent');", 0, 0, 0);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT AVG(score) FROM answers WHERE session_id=1;", -1, &stmt, 0);
    sqlite3_step(stmt);
    double avg = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);

    // (8 + 6 + 10) / 3 = 8.0
    ASSERT(avg >= 7.9 && avg <= 8.1, "Score moyen correct (8.0)");
}

// ── Test 5 : Update session score ──
void test_update_session_score() {
    printf("\n" YELLOW "► Test: Update score session" RESET "\n");

    const char *sql = "UPDATE sessions SET total_score=8.0, badge='Excellent Candidate' WHERE id=1;";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    ASSERT(rc == SQLITE_OK, "Update session réussi");

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT total_score, badge FROM sessions WHERE id=1;", -1, &stmt, 0);
    sqlite3_step(stmt);
    double score = sqlite3_column_double(stmt, 0);
    const char *badge = (const char*)sqlite3_column_text(stmt, 1);
    ASSERT(score == 8.0, "Score mis à jour correctement");
    ASSERT(strcmp(badge, "Excellent Candidate") == 0, "Badge correct");
    sqlite3_finalize(stmt);
}

// ── Test 6 : Apostrophes SQL ──
void test_sql_escape() {
    printf("\n" YELLOW "► Test: Échappement apostrophes" RESET "\n");

    // Tester avec apostrophe dans le texte
    const char *sql = "INSERT INTO answers (session_id, question, answer, score, feedback) VALUES (1, 'L''IA est cool?', 'C''est bon', 7, 'Bien');";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    ASSERT(rc == SQLITE_OK, "Apostrophe échappée correctement");
}

// ── Test 7 : Requête jointure ──
void test_join_query() {
    printf("\n" YELLOW "► Test: Requête JOIN" RESET "\n");

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT u.name, s.domain, s.role "
        "FROM sessions s JOIN users u ON s.user_id = u.id "
        "WHERE s.id = 1;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    ASSERT(rc == SQLITE_OK, "Requête JOIN préparée");
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    ASSERT(found, "JOIN retourne un résultat");
    if (found) {
        ASSERT(strcmp((const char*)sqlite3_column_text(stmt, 0), "TestUser") == 0, "Nom user correct via JOIN");
    }
    sqlite3_finalize(stmt);
}

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     HireMe AI — Tests DB (Jour 22)   ║\n");
    printf("╚══════════════════════════════════════╝\n");

    setup_test_db();

    test_insert_user();
    test_insert_session();
    test_insert_answer();
    test_average_score();
    test_update_session_score();
    test_sql_escape();
    test_join_query();

    teardown_test_db();

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  Résultats : ");
    printf(GREEN "%d passed" RESET " | ", tests_passed);
    printf(RED "%d failed" RESET "\n", tests_failed);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    return tests_failed > 0 ? 1 : 0;
}