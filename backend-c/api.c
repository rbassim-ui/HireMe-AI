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
extern int db_insert_user(const char *name);
extern int db_insert_session(int user_id, const char *domain, const char *role, const char *level);
extern char* db_get_session(int session_id);

// Utility: send JSON HTTP response with CORS
void send_json_response(SOCKET client_socket, int status_code, const char *body) {
    char http_response[2048];
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        status_code, (status_code==200?"OK":"Bad Request"), (int)strlen(body), body);

    send(client_socket, http_response, (int)strlen(http_response), 0);
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
            if (strstr(buffer, "POST /api/session")) {
                // Extraire le body JSON
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

                    get_param(normalized_body, "name", name, sizeof(name));
                    get_param(normalized_body, "domain", domain, sizeof(domain));
                    get_param(normalized_body, "role", role, sizeof(role));
                    get_param(normalized_body, "level", level, sizeof(level));
                    get_param(normalized_body, "startedAt", startedAt, sizeof(startedAt));

                    // Validate required fields
                    if (strlen(name) == 0 || strlen(domain) == 0 || strlen(role) == 0) {
                        char resp[256];
                        snprintf(resp, sizeof(resp), "{\"success\":false, \"message\":\"Missing required fields\"}");
                        send_json_response(client_socket, 400, resp);
                        closesocket(client_socket);
                        continue;
                    }
                    
                    // Sauvegarder en DB
                    int user_id = db_insert_user(name);
                    int session_id = db_insert_session(user_id, domain, role, level);
                    
                    // Préparer la réponse JSON
                    char response_body[512];
                    snprintf(response_body, sizeof(response_body),
                        "{\"success\":true, \"session_id\":%d, \"user_id\":%d, \"message\":\"Session créée avec succès\"}",
                        session_id, user_id);
                    
                    // Envoyer la réponse HTTP
                    send_json_response(client_socket, 200, response_body);
                    printf("✓ Session créée: user=%s, domain=%s, role=%s, level=%s\n", name, domain, role, level);
                }
            } else if (strstr(buffer, "GET /api/session/")) {
                // Récupérer une session
                int session_id = 0;
                sscanf(buffer, "GET /api/session/%d", &session_id);
                
                char *session_data = db_get_session(session_id);
                
                send_json_response(client_socket, 200, session_data);
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
