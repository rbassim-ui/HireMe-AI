/**
 * api.c - API REST Server
 * Simple HTTP server for receiving session data from frontend
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "sqlite3.h"

#define PORT 3000
#define BUFFER_SIZE 4096

extern sqlite3 *db;

// Déclarations des fonctions db.c
extern int db_insert_user(const char *name, const char *password);
extern int db_insert_session(int user_id, const char *domain, const char *role, const char *level);
extern int db_insert_answer(int session_id, const char *question, const char *answer, int score, const char *feedback);
extern char* db_get_session(int session_id);
extern int db_update_session_score(int session_id, float total_score, const char *badge);
extern char* db_get_dashboard_stats();
extern int db_find_user_by_credentials(const char *name, const char *password);
extern int db_user_name_exists(const char *name);
extern int db_update_user_name(int user_id, const char *name);
extern int db_update_user_password(int user_id, const char *password);
extern int db_delete_user_account(int user_id);
extern char* db_get_user_account(int user_id);
extern char* db_get_user_history(int user_id, const char *domain_filter, const char *search_filter, int limit);

// Déclarations des fonctions gemini.c pour Groq AI
extern char* gemini_generate_question(const char *domain, const char *role, const char *level, int question_num);
extern char* gemini_evaluate_answer(const char *question, const char *answer, const char *level);

// Utility: send JSON HTTP response with CORS
void send_json_response(SOCKET client_socket, int status_code, const char *body) {
    const char *status_text = "Bad Request";
    if (status_code == 200) status_text = "OK";
    else if (status_code == 201) status_text = "Created";
    else if (status_code == 204) status_text = "No Content";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 401) status_text = "Unauthorized";
    else if (status_code == 404) status_text = "Not Found";
    else if (status_code == 500) status_text = "Internal Server Error";

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        status_code, status_text, (int)strlen(body));

    send(client_socket, header, (int)strlen(header), 0);
    send(client_socket, body, (int)strlen(body), 0);
}
// Parser simple pour extraire les paramètres POST
int get_content_length(const char *headers) {
    const char *p = strstr(headers, "Content-Length:");
    if (!p) return 0;
    p += strlen("Content-Length:");
    while (*p == ' ' || *p == '\t') p++;
    int len = 0;
    if (sscanf(p, "%d", &len) == 1) return len;
    return 0;
}

int get_query_param(const char *request, const char *param_name, char *out, size_t out_size) {
    if (!request || !param_name || !out || out_size == 0) return 0;
    memset(out, 0, out_size);

    const char *query = strchr(request, '?');
    if (!query) return 0;

    const char *space = strchr(query, ' ');
    if (!space) return 0;

    char key[64];
    snprintf(key, sizeof(key), "%s=", param_name);

    size_t query_len = (size_t)(space - query - 1);
    if (query_len == 0) return 0;

    char query_buf[512];
    if (query_len >= sizeof(query_buf)) query_len = sizeof(query_buf) - 1;
    memcpy(query_buf, query + 1, query_len);
    query_buf[query_len] = '\0';

    char *pos = strstr(query_buf, key);
    if (!pos) return 0;

    pos += strlen(key);
    size_t i = 0;
    while (*pos && *pos != '&' && i + 1 < out_size) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 1;
}

// Parser simple mais robuste pour extraire les paramètres JSON (quoted or unquoted)
int get_param(const char *body, const char *param_name, char *out, size_t out_size) {
    if (!out || out_size == 0) return 0;
    memset(out, 0, out_size);

    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", param_name);
    char *pos = strstr(body, key);
    if (!pos) return 0;

    // find ':' after the key
    char *colon = strchr(pos + strlen(key), ':');
    if (!colon) return 0;
    char *p = colon + 1;
    while (*p == ' ' || *p == '\t') p++;

    if (*p == '"') {
        p++; // skip opening quote
        int i = 0;
        while (*p && *p != '"' && i < (int)out_size - 1) {
            out[i++] = *p++;
        }
        out[i] = '\0';
        return 1;
    }

    // unquoted value (number, boolean, null)
    int i = 0;
    while (*p && *p != ',' && *p != '}' && *p != '\n' && i < (int)out_size - 1) {
        if (*p != ' ' && *p != '\r' && *p != '\t') out[i++] = *p;
        p++;
    }
    out[i] = '\0';
    return 1;
}

void normalize_json_body(const char *src, char *dst, size_t dst_size) {
    size_t i = 0;
    for (size_t j = 0; src[j] != '\0' && i + 1 < dst_size; j++) {
        if (src[j] == '\\' && src[j + 1] == '"') {
            dst[i++] = '"';
            j++;
        } else {
            dst[i++] = src[j];
        }
    }
    dst[i] = '\0';
}

void escape_json_string(const char *src, char *dst, size_t dst_size) {
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

// Démarrer le serveur HTTP
void start_api_server() {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Erreur WSAStartup\n");
        return;
    }
    
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Erreur création socket\n");
        WSACleanup();
        return;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Erreur bind sur port %d\n", PORT);
        closesocket(server_socket);
        WSACleanup();
        return;
    }
    
    listen(server_socket, 1);
    printf("✓ API Server démarré sur http://127.0.0.1:%d\n", PORT);
    
    // Boucle de réception
    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (client_socket == INVALID_SOCKET) continue;
        
        char buffer[BUFFER_SIZE] = {0};
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            // Read until the HTTP body declared by Content-Length is available.
            char *headers_end = strstr(buffer, "\r\n\r\n");
            if (headers_end) {
                int headers_len = (int)(headers_end - buffer) + 4;
                int content_len = get_content_length(buffer);
                int body_bytes = bytes_received - headers_len;

                while (content_len > 0 && body_bytes < content_len && bytes_received < BUFFER_SIZE - 1) {
                    int r = recv(client_socket, buffer + bytes_received, BUFFER_SIZE - 1 - bytes_received, 0);
                    if (r <= 0) {
                        break;
                    }

                    bytes_received += r;
                    body_bytes = bytes_received - headers_len;
                    buffer[bytes_received] = '\0';
                }
            }

            // Parser la requête
            if (strstr(buffer, "POST /api/register")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char name[256];
                    char password[256];
                    get_param(normalized_body, "name", name, sizeof(name));
                    get_param(normalized_body, "password", password, sizeof(password));

                    if (strlen(name) == 0 || strlen(password) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Name and password are required\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (db_user_name_exists(name)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Account already exists\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    int user_id = db_insert_user(name, password);
                    if (user_id <= 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Unable to create account\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true,\"user_id\":%d,\"name\":\"%s\",\"message\":\"Account created\"}",
                        user_id, name);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/login")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char name[256];
                    char password[256];
                    get_param(normalized_body, "name", name, sizeof(name));
                    get_param(normalized_body, "password", password, sizeof(password));

                    int user_id = db_find_user_by_credentials(name, password);
                    if (user_id <= 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Invalid credentials\"}");
                        send_json_response(client_socket, 401, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true,\"user_id\":%d,\"name\":\"%s\",\"message\":\"Login successful\"}",
                        user_id, name);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/account/update-name")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char user_id_str[32];
                    char name[256];
                    get_param(normalized_body, "user_id", user_id_str, sizeof(user_id_str));
                    get_param(normalized_body, "name", name, sizeof(name));

                    int user_id = user_id_str[0] ? atoi(user_id_str) : 0;
                    if (user_id <= 0 || strlen(name) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Invalid account update payload\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (!db_update_user_name(user_id, name)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Failed to update name\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body), "{\"success\":true,\"user_id\":%d,\"name\":\"%s\"}", user_id, name);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/account/change-password")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char user_id_str[32];
                    char password[256];
                    get_param(normalized_body, "user_id", user_id_str, sizeof(user_id_str));
                    get_param(normalized_body, "password", password, sizeof(password));

                    int user_id = user_id_str[0] ? atoi(user_id_str) : 0;
                    if (user_id <= 0 || strlen(password) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Invalid password update payload\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (!db_update_user_password(user_id, password)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Failed to update password\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body), "{\"success\":true,\"user_id\":%d}", user_id);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/account/delete")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char user_id_str[32];
                    get_param(normalized_body, "user_id", user_id_str, sizeof(user_id_str));
                    int user_id = user_id_str[0] ? atoi(user_id_str) : 0;

                    if (user_id <= 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Invalid user_id\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (!db_delete_user_account(user_id)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Failed to delete account\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body), "{\"success\":true,\"user_id\":%d}", user_id);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/session/score")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char session_id_str[32];
                    char total_score_str[32];
                    char badge[256];

                    get_param(normalized_body, "session_id", session_id_str, sizeof(session_id_str));
                    get_param(normalized_body, "total_score", total_score_str, sizeof(total_score_str));
                    get_param(normalized_body, "badge", badge, sizeof(badge));

                    int session_id = session_id_str[0] ? atoi(session_id_str) : 0;
                    float total_score = total_score_str[0] ? (float)atof(total_score_str) : 0.0f;

                    if (session_id <= 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Invalid session_id\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (badge[0] == '\0') {
                        if (total_score >= 8.0f) strcpy(badge, "Excellent Candidate");
                        else if (total_score >= 6.0f) strcpy(badge, "Good Potential");
                        else if (total_score >= 5.0f) strcpy(badge, "Rising Profile");
                        else strcpy(badge, "Needs Practice");
                    }

                    if (!db_update_session_score(session_id, total_score, badge)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Failed to update score\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[1024];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true, \"session_id\":%d, \"total_score\":%.2f, \"badge\":\"%s\"}",
                        session_id, total_score, badge);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "POST /api/session")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char name[256];
                    char domain[256];
                    char role[256];
                    char level[256];
                    char startedAt[256];
                    char user_id_str[32];

                    get_param(normalized_body, "name", name, sizeof(name));
                    get_param(normalized_body, "domain", domain, sizeof(domain));
                    get_param(normalized_body, "role", role, sizeof(role));
                    get_param(normalized_body, "level", level, sizeof(level));
                    get_param(normalized_body, "startedAt", startedAt, sizeof(startedAt));
                    get_param(normalized_body, "user_id", user_id_str, sizeof(user_id_str));

                    if (strlen(name) == 0 || strlen(domain) == 0 || strlen(role) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Missing required fields\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    int user_id = user_id_str[0] ? atoi(user_id_str) : 0;
                    if (user_id <= 0) {
                        user_id = db_insert_user(name, "");
                    }
                    int session_id = db_insert_session(user_id, domain, role, level);

                    char response_body[512];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true, \"session_id\":%d, \"user_id\":%d, \"message\":\"Session créée avec succès\"}",
                        session_id, user_id);

                    send_json_response(client_socket, 200, response_body);
                    printf("✓ Session créée: user=%s, domain=%s, role=%s, level=%s\n", name, domain, role, level);
                }
            } else if (strstr(buffer, "GET /api/account/history")) {
                char user_id_str[32];
                char domain[128];
                char q[128];
                char limit_str[32];

                int has_user_id = get_query_param(buffer, "user_id", user_id_str, sizeof(user_id_str));
                get_query_param(buffer, "domain", domain, sizeof(domain));
                get_query_param(buffer, "q", q, sizeof(q));
                get_query_param(buffer, "limit", limit_str, sizeof(limit_str));

                int user_id = has_user_id ? atoi(user_id_str) : 0;
                int limit = limit_str[0] ? atoi(limit_str) : 50;

                if (user_id <= 0) {
                    char resp[256];
                    snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Missing user_id\"}");
                    send_json_response(client_socket, 400, resp);
                } else {
                    char *history = db_get_user_history(user_id, domain, q, limit);
                    send_json_response(client_socket, 200, history);
                }
            } else if (strstr(buffer, "GET /api/account")) {
                char user_id_str[32];
                if (get_query_param(buffer, "user_id", user_id_str, sizeof(user_id_str))) {
                    int user_id = atoi(user_id_str);
                    char *account = db_get_user_account(user_id);
                    send_json_response(client_socket, 200, account);
                } else {
                    char resp[256];
                    snprintf(resp, sizeof(resp), "{\"success\":false,\"message\":\"Missing user_id\"}");
                    send_json_response(client_socket, 400, resp);
                }
            } else if (strstr(buffer, "GET /api/stats")) {
                char *stats = db_get_dashboard_stats();
                send_json_response(client_socket, 200, stats);
            } else if (strstr(buffer, "GET /api/session/")) {
                int session_id = 0;
                sscanf(buffer, "GET /api/session/%d", &session_id);

                char *session_data = db_get_session(session_id);
                send_json_response(client_socket, 200, session_data);
            } else if (strstr(buffer, "POST /api/question")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char domain[256], role[256], level[256];
                    char question_num_str[32];

                    get_param(normalized_body, "domain", domain, sizeof(domain));
                    get_param(normalized_body, "role", role, sizeof(role));
                    get_param(normalized_body, "level", level, sizeof(level));
                    get_param(normalized_body, "question_num", question_num_str, sizeof(question_num_str));

                    int question_num = question_num_str[0] ? atoi(question_num_str) : 1;
                    char *question = gemini_generate_question(domain, role, level, question_num);

                    if (question && strlen(question) > 0) {
                        char question_escaped[2048];
                        char response_body[2048];
                        escape_json_string(question, question_escaped, sizeof(question_escaped));
                        snprintf(response_body, sizeof(response_body),
                            "{\"success\":true, \"question\":\"%s\"}", question_escaped);
                        send_json_response(client_socket, 200, response_body);
                        printf("✓ Question générée pour %s/%s (%s)\n", domain, role, level);
                    } else {
                        char response_body[512];
                        snprintf(response_body, sizeof(response_body),
                            "{\"success\":false, \"message\":\"Generation question IA indisponible. Verifiez GROQ_API_KEY et la connexion reseau.\"}");
                        send_json_response(client_socket, 500, response_body);
                    }
                }
            } else if (strstr(buffer, "POST /api/evaluate")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char question[1024], answer[1024], level[256];
                    get_param(normalized_body, "question", question, sizeof(question));
                    get_param(normalized_body, "answer", answer, sizeof(answer));
                    get_param(normalized_body, "level", level, sizeof(level));

                    char *evaluation = gemini_evaluate_answer(question, answer, level);
                    if (evaluation && strlen(evaluation) > 0) {
                        char evaluation_escaped[2048];
                        char response_body[2048];
                        escape_json_string(evaluation, evaluation_escaped, sizeof(evaluation_escaped));
                        snprintf(response_body, sizeof(response_body),
                            "{\"success\":true, \"evaluation\":\"%s\"}", evaluation_escaped);
                        send_json_response(client_socket, 200, response_body);
                        printf("✓ Réponse évaluée\n");
                    } else {
                        char response_body[512];
                        snprintf(response_body, sizeof(response_body),
                            "{\"success\":false, \"message\":\"Evaluation IA indisponible. Verifiez GROQ_API_KEY et la connexion reseau.\"}");
                        send_json_response(client_socket, 500, response_body);
                    }
                }
            } else if (strstr(buffer, "POST /api/answer")) {
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4;

                    char normalized_body[BUFFER_SIZE] = {0};
                    normalize_json_body(body_start, normalized_body, sizeof(normalized_body));

                    char session_id_str[32];
                    char question[1024];
                    char answer[2048];
                    char score_str[32];
                    char feedback[1024];

                    get_param(normalized_body, "session_id", session_id_str, sizeof(session_id_str));
                    get_param(normalized_body, "question", question, sizeof(question));
                    get_param(normalized_body, "answer", answer, sizeof(answer));
                    get_param(normalized_body, "score", score_str, sizeof(score_str));
                    get_param(normalized_body, "feedback", feedback, sizeof(feedback));

                    int session_id = session_id_str[0] ? atoi(session_id_str) : 0;
                    int score = score_str[0] ? atoi(score_str) : 0;

                    if (session_id <= 0 || strlen(question) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Invalid answer payload\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    if (!db_insert_answer(session_id, question, answer, score, feedback)) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Failed to save answer\"}");
                        send_json_response(client_socket, 500, resp);
                        closesocket(client_socket);
                        continue;
                    }

                    char response_body[256];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true, \"session_id\":%d, \"score\":%d}",
                        session_id, score);
                    send_json_response(client_socket, 200, response_body);
                }
            } else if (strstr(buffer, "OPTIONS")) {
                // CORS preflight
                char http_response[] =
                    "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: Content-Type\r\n"
                    "\r\n";
                send(client_socket, http_response, strlen(http_response), 0);
            }
        }
        closesocket(client_socket);
    }
    closesocket(server_socket);
    WSACleanup();
}
