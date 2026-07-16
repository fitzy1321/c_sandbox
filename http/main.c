#include <stdio.h>
#include <stdlib.h>
#include "http_client.h"

int main(void) {
    /* Every socket this library opens will now time out after 5s
       instead of blocking forever on an unresponsive server. */
    http_set_timeout(5);
    HTTP_CLIENT_H.g_timeout_seconds = 20;
    /* --- GET example, plus header lookup --- */
    http_response_t *r = http_get("example.com", "/");
    if (r) {
        printf("=== GET example.com/ ===\n");
        printf("Status: %d (%s)\n", r->status_code, r->status_line);
        printf("Body length: %zu bytes\n", r->body_len);

        char *content_type = http_get_header(r->headers, "Content-Type");
        if (content_type) {
            printf("Content-Type: %s\n", content_type);
            free(content_type);
        }
        printf("First 200 bytes of body:\n%.200s\n\n", r->body);
        http_response_free(r);
    } else {
        fprintf(stderr, "GET request failed\n");
    }

    /* --- POST example ---
       httpbin.org/post echoes back whatever you send it, which is
       handy for testing. */
    const char *json_body = "{\"hello\":\"world\"}";
    http_response_t *p = http_post("httpbin.org", "/post", json_body, "application/json");
    if (p) {
        printf("=== POST httpbin.org/post ===\n");
        printf("Status: %d (%s)\n", p->status_code, p->status_line);
        printf("Body:\n%s\n\n", p->body);
        http_response_free(p);
    } else {
        fprintf(stderr, "POST request failed\n");
    }

    /* --- Generic request example: full URL + custom method + custom
       header, no need to pre-split host/path yourself --- */
    http_response_t *d = http_request("DELETE", "http://httpbin.org/delete",
                                       "X-Custom-Header: demo\r\n", NULL, NULL);
    if (d) {
        printf("=== DELETE httpbin.org/delete ===\n");
        printf("Status: %d (%s)\n", d->status_code, d->status_line);
        http_response_free(d);
    } else {
        fprintf(stderr, "DELETE request failed\n");
    }

    /* --- URL parsing demo on its own --- */
    http_url_t parsed;
    if (http_parse_url("http://example.com:8080/foo/bar?x=1", &parsed) == 0) {
        printf("=== Parsed URL ===\n");
        printf("scheme=%s host=%s port=%s path=%s\n",
               parsed.scheme, parsed.host, parsed.port, parsed.path);
    }

    /* An https:// URL will parse fine but http_request() will refuse it
       since this client has no TLS support: */
    http_response_t *bad = http_request("GET", "https://example.com/", NULL, NULL, NULL);
    if (!bad) {
        printf("(https request correctly rejected - no TLS support)\n");
    }

    return 0;
}
