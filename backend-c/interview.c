/**
 * interview.c - Interview simulation engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "sqlite3.h"

#define MAX_QUESTIONS 5
#define MAX_ANSWER_LEN 1024

extern sqlite3 *db;
extern int db_insert_user(const char *name);
extern int db_insert_session(int user_id, const char *domain, const char *role, const char *level);
extern char* gemini_generate_question(const char *domain, const char *role, const char *level, int question_num);
extern char* gemini_evaluate_answer(const char *question, const char *answer, const char *level);
extern int extract_score(const char *response);
extern char* extract_feedback(const char *response);

void escape_sql(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 2; i++) {
        if (src[i] == '\'') {
            dst[j++] = '\'';
            dst[j++] = '\'';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

void save_answer(int session_id, const char *question, const char *answer, int score, const char *feedback) {
    char q_escaped[2048], a_escaped[2048], f_escaped[2048];
    escape_sql(question, q_escaped, sizeof(q_escaped));
    escape_sql(answer,   a_escaped, sizeof(a_escaped));
    escape_sql(feedback, f_escaped, sizeof(f_escaped));

    char sql[4096];
    snprintf(sql, sizeof(sql),
        "INSERT INTO answers (session_id, question, answer, score, feedback) "
        "VALUES (%d, '%s', '%s', %d, '%s');",
        session_id, q_escaped, a_escaped, score, f_escaped);

    char *err = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Erreur save answer: %s\n", err);
        sqlite3_free(err);
    }
}

void update_session_score(int session_id, float avg_score) {
    const char *badge;
    if (avg_score >= 8.0)      badge = "Excellent Candidate";
    else if (avg_score >= 6.0) badge = "Good Potential";
    else                       badge = "Needs Practice";

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE sessions SET total_score=%.2f, badge='%s' WHERE id=%d;",
        avg_score, badge, session_id);

    char *err = 0;
    sqlite3_exec(db, sql, 0, 0, &err);
    if (err) sqlite3_free(err);
}

void save_score_summary(int session_id, float avg_score) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO scores (session_id, average_score, total_questions) VALUES (%d, %.2f, %d);",
        session_id, avg_score, MAX_QUESTIONS);

    char *err = 0;
    sqlite3_exec(db, sql, 0, 0, &err);
    if (err) sqlite3_free(err);
}

void afficher_progression(int current, int total) {
    printf("\n[");
    for (int i = 0; i < total; i++) {
        if (i < current) printf("█");
        else             printf("░");
    }
    printf("] %d/%d\n", current, total);
}

void start_interview() {
    char name[128], domain[64], role[128], level[32];

    printf("\n");
    printf("╔═══════════════════════════════════╗\n");
    printf("║     HireMe AI — Entretien         ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    printf("Votre nom      : ");
    scanf(" %[^\n]", name);
    printf("Domaine        : ");
    scanf(" %[^\n]", domain);
    printf("Rôle           : ");
    scanf(" %[^\n]", role);
    printf("Niveau (Beginner/Intermediate/Advanced): ");
    scanf(" %[^\n]", level);

    int user_id    = db_insert_user(name);
    int session_id = db_insert_session(user_id, domain, role, level);

    printf("\n✓ Session #%d créée pour %s\n", session_id, name);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  Domaine : %s\n", domain);
    printf("  Rôle    : %s\n", role);
    printf("  Niveau  : %s\n", level);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\nAppuyez sur ENTRÉE pour commencer...");
    getchar(); getchar();

    int total_score = 0;
    char answer[MAX_ANSWER_LEN];
    char questions[MAX_QUESTIONS][1024];

    for (int i = 0; i < MAX_QUESTIONS; i++) {
        afficher_progression(i, MAX_QUESTIONS);

        printf("\n⏳ Génération de la question %d/%d...\n", i + 1, MAX_QUESTIONS);
        Sleep(2000);
        char *question = gemini_generate_question(domain, role, level, i + 1);
        strncpy(questions[i], question, sizeof(questions[i]) - 1);

        printf("\n❓ Question %d/%d :\n", i + 1, MAX_QUESTIONS);
        printf("   %s\n\n", questions[i]);
        printf("Votre réponse : ");
        scanf(" %[^\n]", answer);

        printf("\n⏳ Évaluation en cours...\n");
        Sleep(2000);
        char *evaluation = gemini_evaluate_answer(questions[i], answer, level);

        int score      = extract_score(evaluation);
        char *feedback = extract_feedback(evaluation);
        total_score   += score;

        printf("\n  ✓ Score    : %d/10\n", score);
        printf("  💬 Feedback: %s\n", feedback);
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

        save_answer(session_id, questions[i], answer, score, feedback);
    }

    float avg = (float)total_score / MAX_QUESTIONS;
    update_session_score(session_id, avg);
    save_score_summary(session_id, avg);

    afficher_progression(MAX_QUESTIONS, MAX_QUESTIONS);

    const char *badge;
    if (avg >= 8.0)      badge = "Excellent Candidate";
    else if (avg >= 6.0) badge = "Good Potential";
    else                 badge = "Needs Practice";

    printf("\n╔═══════════════════════════════════╗\n");
    printf("║         RÉSULTAT FINAL            ║\n");
    printf("╠═══════════════════════════════════╣\n");
    printf("║  Candidat : %-21s ║\n", name);
    printf("║  Score    : %.1f / 10              ║\n", avg);
    printf("║  Badge    : %-21s ║\n", badge);
    printf("╚═══════════════════════════════════╝\n");
    printf("\n✓ Session sauvegardée dans la base de données.\n");
}