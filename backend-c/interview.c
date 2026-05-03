/**
 * interview.c - Interview simulation engine
 * Manages interview flow, question delivery, and response recording
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

// Questions statiques par domaine (en attendant l'AI au Jour 8-9)
const char *questions_tech[] = {
    "Expliquez la différence entre une pile (stack) et une file (queue).",
    "Qu'est-ce que la complexité algorithmique O(n) ?",
    "Expliquez le concept de pointeur en C.",
    "Quelle est la différence entre TCP et UDP ?",
    "Qu'est-ce qu'une base de données relationnelle ?"
};

const char *questions_data[] = {
    "Qu'est-ce que la régression linéaire ?",
    "Expliquez la différence entre supervised et unsupervised learning.",
    "Qu'est-ce qu'un DataFrame en pandas ?",
    "Comment gérez-vous les valeurs manquantes dans un dataset ?",
    "Qu'est-ce que l'overfitting ?"
};

const char *questions_business[] = {
    "Comment gérez-vous un client difficile ?",
    "Décrivez votre processus de recrutement idéal.",
    "Comment mesurez-vous la performance d'une équipe ?",
    "Qu'est-ce qu'un KPI ? Donnez un exemple.",
    "Comment priorisez-vous vos tâches en cas de surcharge ?"
};

const char *questions_general[] = {
    "Parlez-moi de vous en 2 minutes.",
    "Quelles sont vos principales forces ?",
    "Où vous voyez-vous dans 5 ans ?",
    "Pourquoi voulez-vous rejoindre notre entreprise ?",
    "Décrivez une situation difficile que vous avez surmontée."
};

// Choisir les questions selon le domaine
const char **get_questions(const char *domain) {
    if (strstr(domain, "tech") || strstr(domain, "Tech") || strstr(domain, "data") || strstr(domain, "Data"))
        return questions_tech;
    if (strstr(domain, "data") || strstr(domain, "Data"))
        return questions_data;
    if (strstr(domain, "business") || strstr(domain, "Business") || strstr(domain, "finance") || strstr(domain, "Finance"))
        return questions_business;
    return questions_general;
}

// Sauvegarder une réponse dans la DB
void save_answer(int session_id, const char *question, const char *answer, int score, const char *feedback) {
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "INSERT INTO answers (session_id, question, answer, score, feedback) "
        "VALUES (%d, '%s', '%s', %d, '%s');",
        session_id, question, answer, score, feedback);

    char *err = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        printf("Erreur save answer: %s\n", err);
        sqlite3_free(err);
    }
}

// Mettre à jour le score final de la session
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

// Sauvegarder dans la table scores
void save_score_summary(int session_id, float avg_score) {
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO scores (session_id, average_score, total_questions) VALUES (%d, %.2f, %d);",
        session_id, avg_score, MAX_QUESTIONS);

    char *err = 0;
    sqlite3_exec(db, sql, 0, 0, &err);
    if (err) sqlite3_free(err);
}

// Afficher la barre de progression
void afficher_progression(int current, int total) {
    printf("\n[");
    for (int i = 0; i < total; i++) {
        if (i < current) printf("█");
        else             printf("░");
    }
    printf("] %d/%d\n", current, total);
}

// Lancer l'entretien complet
void start_interview() {
    char name[128];
    char domain[64];
    char role[128];
    char level[32];

    printf("\n");
    printf("╔═══════════════════════════════════╗\n");
    printf("║     HireMe AI — Entretien         ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    // Collecter les infos
    printf("Votre nom      : ");
    scanf(" %[^\n]", name);

    printf("Domaine        : ");
    scanf(" %[^\n]", domain);

    printf("Rôle           : ");
    scanf(" %[^\n]", role);

    printf("Niveau (Beginner/Intermediate/Advanced): ");
    scanf(" %[^\n]", level);

    // Créer user + session en DB
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

    // Choisir les questions
    const char **questions = get_questions(domain);

    int total_score = 0;
    char answer[MAX_ANSWER_LEN];

    // Boucle 5 questions
    for (int i = 0; i < MAX_QUESTIONS; i++) {
        afficher_progression(i, MAX_QUESTIONS);

        printf("\n❓ Question %d/%d :\n", i + 1, MAX_QUESTIONS);
        printf("   %s\n\n", questions[i]);
        printf("Votre réponse : ");
        scanf(" %[^\n]", answer);

        // Score simulé (sera remplacé par AI au Jour 10)
        int score = 5 + (rand() % 6); // score entre 5 et 10
        total_score += score;

        const char *feedback = (score >= 8)
            ? "Bonne réponse, claire et structurée."
            : (score >= 6)
            ? "Réponse correcte mais manque de détails."
            : "Réponse insuffisante, à approfondir.";

        printf("\n  ✓ Score    : %d/10\n", score);
        printf("  💬 Feedback: %s\n", feedback);
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

        // Sauvegarder la réponse
        save_answer(session_id, questions[i], answer, score, feedback);

        Sleep(500);
    }

    // Résultat final
    float avg = (float)total_score / MAX_QUESTIONS;
    update_session_score(session_id, avg);
    save_score_summary(session_id, avg);

    afficher_progression(MAX_QUESTIONS, MAX_QUESTIONS);

    const char *badge;
    if (avg >= 8.0)      badge = "🏆 Excellent Candidate";
    else if (avg >= 6.0) badge = "⭐ Good Potential";
    else                 badge = "📚 Needs Practice";

    printf("\n╔═══════════════════════════════════╗\n");
    printf("║         RÉSULTAT FINAL            ║\n");
    printf("╠═══════════════════════════════════╣\n");
    printf("║  Candidat : %-21s ║\n", name);
    printf("║  Score    : %.1f / 10              ║\n", avg);
    printf("║  Badge    : %-21s ║\n", badge);
    printf("╚═══════════════════════════════════╝\n");
    printf("\n✓ Session sauvegardée dans la base de données.\n");
}