/**
 * test_score.c - Tests calcul des scores
 * Jour 22 : Tests automatiques
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

int tests_passed = 0;
int tests_failed = 0;

#define ASSERT(condition, message) do { \
    if (condition) { \
        printf(GREEN "  ✓ PASS" RESET " — %s\n", message); \
        tests_passed++; \
    } else { \
        printf(RED "  ✗ FAIL" RESET " — %s\n", message); \
        tests_failed++; \
    } \
} while(0)

// Fonctions à tester (copie locale)
int extract_score(const char *response) {
    const char *score_pos = strstr(response, "SCORE:");
    if (!score_pos) score_pos = strstr(response, "Score:");
    if (!score_pos) return 5;
    score_pos += 6;
    while (*score_pos == ' ') score_pos++;
    int score = 0;
    sscanf(score_pos, "%d", &score);
    if (score < 0) score = 0;
    if (score > 10) score = 10;
    return score;
}

const char* get_badge(float avg) {
    if (avg >= 8.0) return "Excellent Candidate";
    if (avg >= 6.0) return "Good Potential";
    return "Needs Practice";
}

float calculate_average(int scores[], int count) {
    if (count == 0) return 0.0;
    int total = 0;
    for (int i = 0; i < count; i++) total += scores[i];
    return (float)total / count;
}

// ── Tests extract_score ──
void test_extract_score() {
    printf("\n" YELLOW "► Test: Extraction score" RESET "\n");

    ASSERT(extract_score("SCORE: 8/10 | FEEDBACK: Bien") == 8, "Score 8 extrait");
    ASSERT(extract_score("SCORE: 10/10 | FEEDBACK: Parfait") == 10, "Score 10 extrait");
    ASSERT(extract_score("SCORE: 0/10 | FEEDBACK: Insuffisant") == 0, "Score 0 extrait");
    ASSERT(extract_score("Score: 7/10 | FEEDBACK: Correct") == 7, "Score minuscule extrait");
    ASSERT(extract_score("Pas de score ici") == 5, "Score défaut = 5");
    ASSERT(extract_score("SCORE: 15/10 | FEEDBACK: ...") == 10, "Score > 10 limité à 10");
    ASSERT(extract_score("SCORE: -3/10 | FEEDBACK: ...") == 0, "Score < 0 limité à 0");
}

// ── Tests badge ──
void test_badge() {
    printf("\n" YELLOW "► Test: Attribution badge" RESET "\n");

    ASSERT(strcmp(get_badge(9.5), "Excellent Candidate") == 0, "Badge Excellent pour 9.5");
    ASSERT(strcmp(get_badge(8.0), "Excellent Candidate") == 0, "Badge Excellent pour 8.0");
    ASSERT(strcmp(get_badge(7.5), "Good Potential") == 0, "Badge Good pour 7.5");
    ASSERT(strcmp(get_badge(6.0), "Good Potential") == 0, "Badge Good pour 6.0");
    ASSERT(strcmp(get_badge(5.9), "Needs Practice") == 0, "Badge Needs Practice pour 5.9");
    ASSERT(strcmp(get_badge(0.0), "Needs Practice") == 0, "Badge Needs Practice pour 0");
}

// ── Tests moyenne ──
void test_average() {
    printf("\n" YELLOW "► Test: Calcul moyenne" RESET "\n");

    int scores1[] = {8, 6, 10, 7, 9};
    ASSERT(calculate_average(scores1, 5) == 8.0, "Moyenne 5 scores = 8.0");

    int scores2[] = {10, 10, 10, 10, 10};
    ASSERT(calculate_average(scores2, 5) == 10.0, "Moyenne parfaite = 10.0");

    int scores3[] = {0, 0, 0, 0, 0};
    ASSERT(calculate_average(scores3, 5) == 0.0, "Moyenne nulle = 0.0");

    int scores4[] = {5};
    ASSERT(calculate_average(scores4, 1) == 5.0, "Moyenne 1 score = 5.0");
}

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║   HireMe AI — Tests Score (Jour 22)  ║\n");
    printf("╚══════════════════════════════════════╝\n");

    test_extract_score();
    test_badge();
    test_average();

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  Résultats : ");
    printf(GREEN "%d passed" RESET " | ", tests_passed);
    printf(RED "%d failed" RESET "\n", tests_failed);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    return tests_failed > 0 ? 1 : 0;
}