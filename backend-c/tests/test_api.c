/**
 * test_api.c - Tests parsing JSON
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

// Fonction à tester
int is_api_error(const char *json) {
    return strstr(json, "\"error\"") != NULL;
}

char* extract_feedback(const char *response) {
    static char feedback[1024];
    const char *fb_pos = strstr(response, "FEEDBACK:");
    if (!fb_pos) fb_pos = strstr(response, "Feedback:");
    if (!fb_pos) {
        strncpy(feedback, response, sizeof(feedback) - 1);
        return feedback;
    }
    fb_pos += 9;
    while (*fb_pos == ' ') fb_pos++;
    strncpy(feedback, fb_pos, sizeof(feedback) - 1);
    feedback[sizeof(feedback) - 1] = '\0';
    return feedback;
}

// ── Tests détection erreur API ──
void test_api_error_detection() {
    printf("\n" YELLOW "► Test: Détection erreur API" RESET "\n");

    ASSERT(is_api_error("{\"error\":{\"code\":429}}") == 1, "Erreur 429 détectée");
    ASSERT(is_api_error("{\"error\":{\"code\":404}}") == 1, "Erreur 404 détectée");
    ASSERT(is_api_error("{\"choices\":[{\"message\":{\"content\":\"Hello\"}}]}") == 0, "Réponse valide non erreur");
    ASSERT(is_api_error("{}") == 0, "JSON vide non erreur");
}

// ── Tests extraction feedback ──
void test_extract_feedback() {
    printf("\n" YELLOW "► Test: Extraction feedback" RESET "\n");

    const char *r1 = "SCORE: 8/10 | FEEDBACK: Très bonne réponse.";
    ASSERT(strcmp(extract_feedback(r1), "Très bonne réponse.") == 0, "Feedback extrait correctement");

    const char *r2 = "SCORE: 5/10 | Feedback: Réponse correcte mais incomplète.";
    ASSERT(strcmp(extract_feedback(r2), "Réponse correcte mais incomplète.") == 0, "Feedback minuscule extrait");

    const char *r3 = "Pas de feedback ici";
    ASSERT(strcmp(extract_feedback(r3), "Pas de feedback ici") == 0, "Sans FEEDBACK: retourne tout");
}

// ── Tests format JSON Groq ──
void test_json_format() {
    printf("\n" YELLOW "► Test: Format JSON Groq" RESET "\n");

    const char *valid_groq = "{\"choices\":[{\"message\":{\"content\":\"Bonjour\"}}]}";
    ASSERT(strstr(valid_groq, "\"content\"") != NULL, "Format Groq contient content");
    ASSERT(strstr(valid_groq, "\"choices\"") != NULL, "Format Groq contient choices");
    ASSERT(is_api_error(valid_groq) == 0, "Format Groq valide pas d'erreur");

    const char *error_groq = "{\"error\":{\"message\":\"Rate limit\",\"code\":429}}";
    ASSERT(is_api_error(error_groq) == 1, "Erreur Groq détectée");
    ASSERT(strstr(error_groq, "Rate limit") != NULL, "Message erreur présent");
}

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║   HireMe AI — Tests API (Jour 22)    ║\n");
    printf("╚══════════════════════════════════════╝\n");

    test_api_error_detection();
    test_extract_feedback();
    test_json_format();

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  Résultats : ");
    printf(GREEN "%d passed" RESET " | ", tests_passed);
    printf(RED "%d failed" RESET "\n", tests_failed);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    return tests_failed > 0 ? 1 : 0;
}