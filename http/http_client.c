#include "http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strncasecmp */
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

/* Applied to every socket via SO_RCVTIMEO/SO_SNDTIMEO. 10s default. */
static int g_timeout_seconds = 10;

void http_set_timeout(int seconds) {
    g_timeout_seconds = seconds;
}

/* ---------------------------------------------------------------- */
/* Open a TCP connection to host:port. Returns a socket fd, or -1.  */
/* Tries both IPv4 and IPv6 addresses (whichever getaddrinfo returns */
/* first and succeeds), and applies the configured send/recv timeout.*/
/* ---------------------------------------------------------------- */
int http_connect(const char *host, const char *port) {
    struct addrinfo hints = {0}, *res, *rp;
    hints.ai_family   = AF_UNSPEC;     /* IPv4 or IPv6, whichever resolves */
    hints.ai_socktype = SOCK_STREAM;   /* TCP */

    int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo(%s:%s): %s\n", host, port, gai_strerror(err));
        return -1;
    }

    int sock = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;

        if (g_timeout_seconds > 0) {
            struct timeval tv;
            tv.tv_sec = g_timeout_seconds;
            tv.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break; /* success */
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) {
        fprintf(stderr, "http_connect: could not connect to %s:%s\n", host, port);
    }
    return sock;
}

/* ---------------------------------------------------------------- */
/* Build and send an HTTP/1.1 request. Returns 0 on success, -1 on   */
/* failure. `extra_headers` and `body` may be NULL.                 */
/* ---------------------------------------------------------------- */
int http_send_request(int sock, const char *method, const char *host,
                       const char *path, const char *extra_headers,
                       const char *body, const char *content_type) {
    char request[8192];
    size_t body_len = body ? strlen(body) : 0;

    int n = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "%s%s%s"
        "%s%zu\r\n"
        "%s"          /* caller-supplied extra headers, if any */
        "\r\n",
        method, path, host,
        content_type ? "Content-Type: " : "", content_type ? content_type : "", content_type ? "\r\n" : "",
        body ? "Content-Length: " : "", body ? body_len : (size_t)0,
        extra_headers ? extra_headers : "");

    if (n < 0 || (size_t)n >= sizeof(request)) {
        fprintf(stderr, "http_send_request: request too large for buffer\n");
        return -1;
    }

    /* Send headers */
    ssize_t sent = send(sock, request, (size_t)n, 0);
    if (sent < 0 || (size_t)sent != (size_t)n) {
        perror("send (headers)");
        return -1;
    }

    /* Send body, if any */
    if (body && body_len > 0) {
        size_t total_sent = 0;
        while (total_sent < body_len) {
            ssize_t s = send(sock, body + total_sent, body_len - total_sent, 0);
            if (s < 0) {
                perror("send (body)");
                return -1;
            }
            total_sent += (size_t)s;
        }
    }

    return 0;
}

/* ---------------------------------------------------------------- */
/* Read from the socket until the peer closes the connection.       */
/* Returns a malloc'd, NUL-terminated buffer; *out_len is the number */
/* of bytes actually received (excludes the extra NUL).             */
/* ---------------------------------------------------------------- */
char *http_receive_all(int sock, size_t *out_len) {
    size_t capacity = 4096;
    size_t len = 0;
    char *buf = malloc(capacity);
    if (!buf) return NULL;

    char chunk[4096];
    ssize_t n;
    while ((n = recv(sock, chunk, sizeof(chunk), 0)) > 0) {
        if (len + (size_t)n + 1 > capacity) {
            capacity *= 2;
            char *tmp = realloc(buf, capacity);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
        memcpy(buf + len, chunk, (size_t)n);
        len += (size_t)n;
    }
    if (n < 0) {
        perror("recv");
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    if (out_len) *out_len = len;
    return buf;
}

/* ---------------------------------------------------------------- */
/* Split a raw HTTP response into status line / headers / body.     */
/* Takes ownership of `raw` (stores it internally); caller must NOT */
/* free `raw` separately - http_response_free() handles it.         */
/* ---------------------------------------------------------------- */
http_response_t *http_parse_response(char *raw, size_t raw_len) {
    http_response_t *resp = calloc(1, sizeof(*resp));
    if (!resp) return NULL;

    /* Header/body split: HTTP uses a blank line (\r\n\r\n) as the boundary */
    char *sep = strstr(raw, "\r\n\r\n");
    size_t header_block_len = sep ? (size_t)(sep - raw) : raw_len;
    char *body_start = sep ? sep + 4 : raw + raw_len;

    /* Copy the header block so we can safely tokenize it */
    char *header_block = malloc(header_block_len + 1);
    if (!header_block) { free(resp); return NULL; }
    memcpy(header_block, raw, header_block_len);
    header_block[header_block_len] = '\0';
    resp->headers = header_block;

    /* First line of the header block is the status line, e.g.
       "HTTP/1.1 200 OK" */
    char *line_end = strstr(header_block, "\r\n");
    size_t status_line_len = line_end ? (size_t)(line_end - header_block) : header_block_len;
    resp->status_line = malloc(status_line_len + 1);
    if (resp->status_line) {
        memcpy(resp->status_line, header_block, status_line_len);
        resp->status_line[status_line_len] = '\0';
    }

    /* Pull the numeric status code out of "HTTP/1.1 200 OK" */
    resp->status_code = 0;
    char *code_ptr = strchr(header_block, ' ');
    if (code_ptr) resp->status_code = atoi(code_ptr + 1);

    /* Body: whatever followed the blank line. Note this is the raw
       body as received - if the server used chunked transfer encoding,
       it will still contain chunk-size markers; this simple client
       does not de-chunk it. */
    resp->body_len = raw_len - (size_t)(body_start - raw);
    resp->body = body_start; /* points into `raw`, which we now own */

    /* Stash the original buffer pointer at the end so free() can find it.
       Simplest approach: store it in headers' companion field via a
       second allocation trick is overkill - instead we just keep `raw`
       alive by never freeing header_block/body separately from it.
       To keep this simple and correct, we duplicate body too: */
    char *body_copy = malloc(resp->body_len + 1);
    if (body_copy) {
        memcpy(body_copy, body_start, resp->body_len);
        body_copy[resp->body_len] = '\0';
    }
    resp->body = body_copy;

    free(raw); /* headers and body were copied out above */
    return resp;
}

void http_response_free(http_response_t *resp) {
    if (!resp) return;
    free(resp->status_line);
    free(resp->headers);
    free(resp->body);
    free(resp);
}

void http_close(int sock) {
    if (sock >= 0) close(sock);
}

/* ---------------------------------------------------------------- */
/* High-level helpers                                                */
/* ---------------------------------------------------------------- */
http_response_t *http_get(const char *host, const char *path) {
    int sock = http_connect(host, "80");
    if (sock < 0) return NULL;

    if (http_send_request(sock, "GET", host, path, NULL, NULL, NULL) != 0) {
        http_close(sock);
        return NULL;
    }

    size_t len;
    char *raw = http_receive_all(sock, &len);
    http_close(sock);
    if (!raw) return NULL;

    return http_parse_response(raw, len);
}

http_response_t *http_post(const char *host, const char *path,
                            const char *body, const char *content_type) {
    int sock = http_connect(host, "80");
    if (sock < 0) return NULL;

    if (http_send_request(sock, "POST", host, path, NULL, body, content_type) != 0) {
        http_close(sock);
        return NULL;
    }

    size_t len;
    char *raw = http_receive_all(sock, &len);
    http_close(sock);
    if (!raw) return NULL;

    return http_parse_response(raw, len);
}

/* ---------------------------------------------------------------- */
/* Parse a URL of the form scheme://host[:port]/path into its parts. */
/* Defaults: port 80 for http, 443 for https; path "/" if omitted.   */
/* Note: this client only ever connects over plain TCP (http_connect */
/* has no TLS support), so http_request() below refuses "https" URLs */
/* even though this parser will happily recognize them.              */
/* ---------------------------------------------------------------- */
int http_parse_url(const char *url, http_url_t *out) {
    if (!url || !out) return -1;
    memset(out, 0, sizeof(*out));

    const char *p = strstr(url, "://");
    if (!p) return -1; /* no scheme */

    size_t scheme_len = (size_t)(p - url);
    if (scheme_len == 0 || scheme_len >= sizeof(out->scheme)) return -1;
    memcpy(out->scheme, url, scheme_len);
    out->scheme[scheme_len] = '\0';

    const char *host_start = p + 3;
    const char *path_start = strchr(host_start, '/');
    const char *host_end = path_start ? path_start : host_start + strlen(host_start);

    /* Look for an optional ":port" within the host segment */
    const char *colon = memchr(host_start, ':', (size_t)(host_end - host_start));
    const char *host_only_end = colon ? colon : host_end;

    size_t host_len = (size_t)(host_only_end - host_start);
    if (host_len == 0 || host_len >= sizeof(out->host)) return -1;
    memcpy(out->host, host_start, host_len);
    out->host[host_len] = '\0';

    if (colon) {
        size_t port_len = (size_t)(host_end - (colon + 1));
        if (port_len == 0 || port_len >= sizeof(out->port)) return -1;
        memcpy(out->port, colon + 1, port_len);
        out->port[port_len] = '\0';
    } else {
        /* No explicit port: default based on scheme */
        const char *default_port = (strcasecmp(out->scheme, "https") == 0) ? "443" : "80";
        strncpy(out->port, default_port, sizeof(out->port) - 1);
    }

    if (path_start) {
        size_t path_len = strlen(path_start);
        if (path_len >= sizeof(out->path)) return -1;
        memcpy(out->path, path_start, path_len);
        out->path[path_len] = '\0';
    } else {
        strcpy(out->path, "/");
    }

    return 0;
}

/* ---------------------------------------------------------------- */
/* Case-insensitive header lookup from a raw "\r\n"-separated header  */
/* block. Returns a malloc'd copy of the value (leading space         */
/* trimmed), or NULL if the header isn't present.                    */
/* ---------------------------------------------------------------- */
char *http_get_header(const char *headers, const char *name) {
    if (!headers || !name) return NULL;

    size_t name_len = strlen(name);
    const char *line = headers;

    while (line && *line) {
        const char *line_end = strstr(line, "\r\n");
        size_t line_len = line_end ? (size_t)(line_end - line) : strlen(line);

        if (line_len > name_len && line[name_len] == ':' &&
            strncasecmp(line, name, name_len) == 0) {
            const char *val_start = line + name_len + 1;
            while (*val_start == ' ') val_start++; /* skip ": " space */
            const char *val_end = line + line_len;
            if (val_end > val_start) {
                size_t val_len = (size_t)(val_end - val_start);
                char *value = malloc(val_len + 1);
                if (!value) return NULL;
                memcpy(value, val_start, val_len);
                value[val_len] = '\0';
                return value;
            }
            return NULL;
        }

        if (!line_end) break;
        line = line_end + 2; /* skip past \r\n to next line */
    }

    return NULL;
}

/* ---------------------------------------------------------------- */
/* Generic request: arbitrary method + full URL + optional extra     */
/* headers. Rejects https:// since we have no TLS support.           */
/* ---------------------------------------------------------------- */
http_response_t *http_request(const char *method, const char *url,
                               const char *extra_headers,
                               const char *body, const char *content_type) {
    http_url_t parsed;
    if (http_parse_url(url, &parsed) != 0) {
        fprintf(stderr, "http_request: could not parse URL: %s\n", url);
        return NULL;
    }

    if (strcasecmp(parsed.scheme, "http") != 0) {
        fprintf(stderr,
            "http_request: scheme '%s' is not supported (this client has no "
            "TLS support, so only plain http:// URLs will work)\n",
            parsed.scheme);
        return NULL;
    }

    int sock = http_connect(parsed.host, parsed.port);
    if (sock < 0) return NULL;

    if (http_send_request(sock, method, parsed.host, parsed.path,
                           extra_headers, body, content_type) != 0) {
        http_close(sock);
        return NULL;
    }

    size_t len;
    char *raw = http_receive_all(sock, &len);
    http_close(sock);
    if (!raw) return NULL;

    return http_parse_response(raw, len);
}
