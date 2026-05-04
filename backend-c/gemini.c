/**
 * gemini.c - Groq API integration
 * Strict mode: Groq generates questions and evaluates answers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

#define GROQ_MODEL        "llama-3.3-70b-versatile"
#define MAX_RESPONSE_SIZE 8192
#define MAX_RETRIES       3
#define RETRY_DELAY_MS    2000

#ifndef GEMINI_API_KEY
#define GEMINI_API_KEY ""
#endif

static int read_key_from_env_file(const char *path, char *out, size_t out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        const char *prefix = "GROQ_API_KEY=";
        if (strncmp(line, prefix, strlen(prefix)) == 0) {
            char *value = line + strlen(prefix);
            while (*value == ' ' || *value == '\t') value++;

            size_t len = strlen(value);
            while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r' || value[len - 1] == ' ' || value[len - 1] == '\t')) {
                value[--len] = '\0';
            }

            if (len >= 2 && ((value[0] == '"' && value[len - 1] == '"') || (value[0] == '\'' && value[len - 1] == '\''))) {
                value[len - 1] = '\0';
                value++;
                len -= 2;
            }

            if (len > 0) {
                strncpy(out, value, out_size - 1);
                out[out_size - 1] = '\0';
                fclose(f);
                return 1;
            }
        }
    }

    fclose(f);
    return 0;
}

static int read_key_from_plain_file(const char *path, char *out, size_t out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (!fgets(out, (int)out_size, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    size_t len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r' || out[len - 1] == ' ' || out[len - 1] == '\t')) {
        out[--len] = '\0';
    }

    return len > 0;
}

static const char* get_groq_api_key(void) {
    static char file_key[512];
    const char *env = getenv("GROQ_API_KEY");
    if (env && env[0] != '\0') return env;
    if (read_key_from_env_file(".env", file_key, sizeof(file_key))) return file_key;
    if (read_key_from_env_file("..\\.env", file_key, sizeof(file_key))) return file_key;
    if (read_key_from_plain_file("groq_api_key.txt", file_key, sizeof(file_key))) return file_key;
    if (read_key_from_plain_file("..\\groq_api_key.txt", file_key, sizeof(file_key))) return file_key;
    if (GEMINI_API_KEY[0] != '\0') return GEMINI_API_KEY;
    return NULL;
}

static int starts_with(const char *text, const char *prefix) {
    if (!text || !prefix) return 0;
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static void json_escape_string(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    if (!src || !dst || dst_size == 0) return;

    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_size; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"' || c == '\\') {
            if (j + 2 >= dst_size) break;
            dst[j++] = '\\';
            dst[j++] = (char)c;
        } else if (c == '\n') {
            if (j + 2 >= dst_size) break;
            dst[j++] = '\\';
            dst[j++] = 'n';
        } else if (c == '\r') {
            if (j + 2 >= dst_size) break;
            dst[j++] = '\\';
            dst[j++] = 'r';
        } else if (c == '\t') {
            if (j + 2 >= dst_size) break;
            dst[j++] = '\\';
            dst[j++] = 't';
        } else if (c < 0x20) {
            continue;
        } else {
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
}

static int groq_post_shell_curl(const char *json_body, char *response, size_t response_size) {
    char temp_dir[MAX_PATH];
    char req_path[MAX_PATH];
    char cmd[4096];
    FILE *req = NULL;
    FILE *pipe = NULL;
    size_t total = 0;
    const char *api_key = get_groq_api_key();

    if (!api_key || api_key[0] == '\0') {
        snprintf(response, response_size,
            "Erreur: GROQ_API_KEY manquante. Definissez GROQ_API_KEY avant de lancer le backend.");
        return 0;
    }

    if (GetTempPathA((DWORD)sizeof(temp_dir), temp_dir) == 0) {
        snprintf(response, response_size, "Erreur: dossier temporaire introuvable.");
        return 0;
    }

    snprintf(req_path, sizeof(req_path), "%shireme_groq_req.json", temp_dir);

    req = fopen(req_path, "wb");
    if (!req) {
        snprintf(response, response_size, "Erreur: impossible d'ecrire le fichier temporaire de requete.");
        return 0;
    }

    fwrite(json_body, 1, strlen(json_body), req);
    fclose(req);

    snprintf(cmd, sizeof(cmd),
        "curl -sS -X POST \"https://api.groq.com/openai/v1/chat/completions\" "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer %s\" "
        "--data-binary @\"%s\"",
        api_key,
        req_path);

    response[0] = '\0';
    pipe = popen(cmd, "r");
    if (!pipe) {
        remove(req_path);
        snprintf(response, response_size, "Erreur: impossible d'executer curl.");
        return 0;
    }

    while (!feof(pipe) && total + 1 < response_size) {
        size_t n = fread(response + total, 1, response_size - total - 1, pipe);
        if (n == 0) break;
        total += n;
    }
    response[total] = '\0';

    pclose(pipe);
    remove(req_path);

    if (response[0] == '\0') {
        snprintf(response, response_size, "Erreur: reponse vide de Groq/curl.");
        return 0;
    }

    return 1;
}

int is_api_error(const char *json) {
    return strstr(json, "\"error\"") != NULL;
}

void extract_error_message(const char *json, char *out, size_t out_size) {
    const char *marker = "\"message\":\"";
    char *pos = strstr(json, marker);
    if (!pos) {
        marker = "\"message\": \"";
        pos = strstr(json, marker);
    }
    if (!pos) {
        strncpy(out, "Erreur API inconnue", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    pos += strlen(marker);
    size_t i = 0;
    while (*pos && *pos != '"' && i + 1 < out_size) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
}

char* extract_text_from_response(const char *json) {
    static char result[MAX_RESPONSE_SIZE];
    memset(result, 0, sizeof(result));

    if (is_api_error(json)) {
        char err_msg[512];
        extract_error_message(json, err_msg, sizeof(err_msg));
        snprintf(result, sizeof(result), "API Error: %s", err_msg);
        return result;
    }

    const char *markers[] = {
        "\"content\": \"",
        "\"content\":\"",
        "\"text\": \"",
        "\"text\":\"",
        NULL
    };

    char *pos = NULL;
    for (int i = 0; markers[i] != NULL; i++) {
        pos = strstr(json, markers[i]);
        if (pos) {
            pos += strlen(markers[i]);
            break;
        }
    }

    if (!pos) {
        snprintf(result, sizeof(result), "Erreur: format de reponse inattendu");
        return result;
    }

    size_t i = 0;
    while (*pos && i + 1 < sizeof(result)) {
        if (*pos == '\\') {
            pos++;
            if (*pos == 'n') result[i++] = '\n';
            else if (*pos == 't') result[i++] = '\t';
            else if (*pos == 'r') result[i++] = '\r';
            else if (*pos == '"') result[i++] = '"';
            else if (*pos == '\\') result[i++] = '\\';
            else if (*pos) result[i++] = *pos;
            if (*pos) pos++;
        } else if (*pos == '"') {
            break;
        } else {
            result[i++] = *pos++;
        }
    }
    result[i] = '\0';

    return result;
}

char* gemini_call(const char *prompt) {
    static char final_result[MAX_RESPONSE_SIZE];
    char escaped_prompt[3072];
    char body[4096];
    char raw_response[MAX_RESPONSE_SIZE];

    json_escape_string(prompt ? prompt : "", escaped_prompt, sizeof(escaped_prompt));

    snprintf(body, sizeof(body),
        "{\"model\":\"%s\","
        "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
        "\"max_tokens\":500,"
        "\"temperature\":0.7}",
        GROQ_MODEL,
        escaped_prompt);

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        int ok = groq_post_shell_curl(body, raw_response, sizeof(raw_response));
        if (!ok) {
            strncpy(final_result, raw_response, sizeof(final_result) - 1);
            final_result[sizeof(final_result) - 1] = '\0';
            if (attempt < MAX_RETRIES) {
                Sleep(RETRY_DELAY_MS);
                continue;
            }
            return final_result;
        }

        if (strstr(raw_response, "\"code\":429") || strstr(raw_response, "rate_limit")) {
            snprintf(final_result, sizeof(final_result), "Erreur: limite de requetes atteinte (Groq).");
            if (attempt < MAX_RETRIES) {
                Sleep(RETRY_DELAY_MS);
                continue;
            }
            return final_result;
        }

        char *text = extract_text_from_response(raw_response);
        strncpy(final_result, text, sizeof(final_result) - 1);
        final_result[sizeof(final_result) - 1] = '\0';
        return final_result;
    }

    snprintf(final_result, sizeof(final_result), "Erreur: impossible de contacter Groq.");
    return final_result;
}

char* gemini_generate_question(const char *domain, const char *role, const char *level, int question_num) {
    static char previous_questions[5][512] = {0};
    char history[1024] = "";
    char prompt[1400];

    if (question_num < 1) question_num = 1;
    if (question_num > 5) question_num = 5;

    for (int i = 0; i < question_num - 1 && i < 5; i++) {
        if (previous_questions[i][0] != '\0') {
            if (strlen(history) + strlen(previous_questions[i]) + 4 < sizeof(history)) {
                strcat(history, previous_questions[i]);
                strcat(history, " | ");
            }
        }
    }

    if (history[0] != '\0') {
        snprintf(prompt, sizeof(prompt),
            "Tu es un recruteur expert. Genere UNE question d'entretien en francais. "
            "Contexte: domaine='%s', role='%s', niveau='%s', numero=%d/5. "
            "Interdit: repetition/paraphrase de ces questions deja posees: [%s]. "
            "Retourne uniquement la question, sans titre, sans numerotation.",
            domain ? domain : "General",
            role ? role : "General",
            level ? level : "Intermediaire",
            question_num,
            history);
    } else {
        snprintf(prompt, sizeof(prompt),
            "Tu es un recruteur expert. Genere UNE question d'entretien en francais. "
            "Contexte: domaine='%s', role='%s', niveau='%s', numero=%d/5. "
            "Retourne uniquement la question, sans titre, sans numerotation.",
            domain ? domain : "General",
            role ? role : "General",
            level ? level : "Intermediaire",
            question_num);
    }

    char *result = gemini_call(prompt);
    if (!result || starts_with(result, "Erreur:") || starts_with(result, "API Error:")) {
        return NULL;
    }

    strncpy(previous_questions[question_num - 1], result, sizeof(previous_questions[0]) - 1);
    previous_questions[question_num - 1][sizeof(previous_questions[0]) - 1] = '\0';
    return result;
}

char* gemini_evaluate_answer(const char *question, const char *answer, const char *level) {
    static char fallback_error[512];
    char prompt[1800];

    snprintf(prompt, sizeof(prompt),
        "Tu es un evaluateur d'entretien. Evalue la reponse en francais pour un niveau '%s'. "
        "Question: %s "
        "Reponse candidat: %s "
        "Donne STRICTEMENT ce format: SCORE: X/10 | FEEDBACK: ... "
        "Regles: si reponse vide, hors-sujet, ou 'je ne sais pas', donne un score tres faible (0 ou 1).",
        level ? level : "Intermediaire",
        question ? question : "",
        answer ? answer : "");

    char *result = gemini_call(prompt);
    if (!result || starts_with(result, "Erreur:") || starts_with(result, "API Error:")) {
        snprintf(fallback_error, sizeof(fallback_error),
            "SCORE: 0/10 | FEEDBACK: Evaluation IA indisponible (Groq). Verifiez GROQ_API_KEY et la connexion reseau.");
        return fallback_error;
    }
    return result;
}

int extract_score(const char *response) {
    const char *score_pos = strstr(response, "SCORE:");
    if (!score_pos) score_pos = strstr(response, "Score:");
    if (!score_pos) return 0;

    score_pos += 6;
    while (*score_pos == ' ') score_pos++;

    int score = 0;
    sscanf(score_pos, "%d", &score);
    if (score < 0) score = 0;
    if (score > 10) score = 10;
    return score;
}

char* extract_feedback(const char *response) {
    static char feedback[1024];
    const char *fb_pos = strstr(response, "FEEDBACK:");
    if (!fb_pos) fb_pos = strstr(response, "Feedback:");
    if (!fb_pos) {
        strncpy(feedback, response, sizeof(feedback) - 1);
        feedback[sizeof(feedback) - 1] = '\0';
        return feedback;
    }

    fb_pos += 9;
    while (*fb_pos == ' ') fb_pos++;
    strncpy(feedback, fb_pos, sizeof(feedback) - 1);
    feedback[sizeof(feedback) - 1] = '\0';
    return feedback;
}
