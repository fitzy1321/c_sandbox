#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>

/* Parsed HTTP response returned to the caller. */
typedef struct {
    int    status_code;   /* e.g. 200, 404 */
    char  *status_line;   /* e.g. "HTTP/1.1 200 OK" */
    char  *headers;       /* raw header block (not parsed field-by-field) */
    char  *body;          /* response body, always NUL-terminated */
    size_t body_len;      /* body length in bytes (in case of embedded NULs) */
} http_response_t;

/* Parsed URL: scheme is recorded but only "http" is actually usable -
 * see http_parse_url()'s comment about https. */
typedef struct {
    char scheme[8];   /* "http" or "https" */
    char host[256];
    char port[8];     /* e.g. "80"; defaulted based on scheme if absent */
    char path[1024];  /* e.g. "/foo/bar?x=1"; defaults to "/" */
} http_url_t;

/* Send/receive timeout in seconds, applied to every socket this library
 * opens. 0 means "no timeout" (blocks forever, the old behavior).
 * Default is 10 seconds - call this once at startup to change it. */
void http_set_timeout(int seconds);

/* Parse a URL like "http://example.com:8080/foo?bar=1" into its parts.
 * Returns 0 on success, -1 on a malformed URL. */
int http_parse_url(const char *url, http_url_t *out);

/* Case-insensitive lookup of a header value from a raw header block
 * (http_response_t::headers). Returns a newly malloc'd copy of the
 * value (caller must free()), or NULL if not found. */
char *http_get_header(const char *headers, const char *name);

/* Low-level building blocks */
int   http_connect(const char *host, const char *port);
int   http_send_request(int sock, const char *method, const char *host,
                         const char *path, const char *extra_headers,
                         const char *body, const char *content_type);
char *http_receive_all(int sock, size_t *out_len);
http_response_t *http_parse_response(char *raw, size_t raw_len);
void  http_response_free(http_response_t *resp);
void  http_close(int sock);

/* High-level one-call helpers (each opens a connection, does the
 * request, parses the response, and closes the socket for you). */
http_response_t *http_get(const char *host, const char *path);
http_response_t *http_post(const char *host, const char *path,
                            const char *body, const char *content_type);

/* Generic version: arbitrary method (GET/PUT/DELETE/PATCH/...), and
 * arbitrary extra headers (e.g. "Authorization: Bearer xyz\r\n"),
 * accepting a full URL rather than a pre-split host/path. Rejects
 * https:// URLs since this client can't speak TLS - see http_parse_url. */
http_response_t *http_request(const char *method, const char *url,
                               const char *extra_headers,
                               const char *body, const char *content_type);

#endif /* HTTP_CLIENT_H */
