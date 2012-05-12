#include "httpd.h"
#include "list.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

static list_t *strings = NULL;

static char *newstring(int length)
{
    char *r = calloc(length+1, 1);

    if (r == NULL) {
        fatal_error("out of memory");
    }
    if (strings == NULL) {
        strings = list_create(compare_strings);
    }
    list_addlast(strings, r);
    return r;
}

static void freestrings()
{
	if (strings != NULL) {
		list_iter_t *it = list_createiter(strings);
		while (list_hasnext(it)) {
			free(list_next(it));
		}
		list_destroyiter(it);
		list_destroy(strings);
		strings = NULL;
	}
}

static char *stripstring(char *s, int len)
{
    char *r;
    char *end = s+len-1;

    while (end >= s && isspace(*end)) end--;
    while (s <= end && isspace(*s)) s++;
    r = newstring(end-s+1);
    strncpy(r, s, end-s+1);
    return r;
}

static int splitstring(char *s, int sep, char **left, char **right)
{
    char *p = strchr(s, sep);
    if (p == NULL)
        return 0;
    *left = stripstring(s, p-s);
    *right = stripstring(p+1, strlen(p+1));
    return 1;
}

static int hexdigit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    fatal_error("bad hex digit");
	return 1;
}

static char *urldecode(char *s)
{
    char *r, *p;

    r = p = newstring(strlen(s));
    for (;;) {
        int ch = *s++;
        switch(ch) {
        case 0:
            return r;
        case '+':
            *p++ = ' ';
            break;
        case '%':
			*p    = hexdigit(*s++) << 4;
			*p++ += hexdigit(*s++);
            break;
        default:
            *p++ = ch;
            break;
        }
    }
}

char *html_escape(char *s)
{
    char *r, *p;

    r = p = newstring(strlen(s)*6);
    for (;;) {
        int ch = *s++;
        switch(ch) {
        case 0:
            return r;
        case '<':
            strcat(p, "&lt;");
            p += 4;
            break;
        case '>':
            strcat(p, "&gt;");
            p += 4;
            break;
        case '&':
            strcat(p, "&amp;");
            p += 5;
            break;
        case '"':
            strcat(p, "&quot;");
            p += 6;
            break;
        default:
            *p++ = ch;
            break;
        }
    }
}

char *js_escape(char *s)
{
    char *r, *p;

    r = p = newstring(strlen(s)*2);
    for (;;) {
        int ch = *s++;
        switch(ch) {
            case 0:
                return r;
            case '\'':
                strcat(p, "\\'");
                p += 2;
                break;
            default:
                *p++ = ch;
                break;
        }
    }
}

void http_ok(FILE *f, char *content_type)
{
    fprintf(f, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
}

void http_notfound(FILE *f, char *path)
{
    fprintf(f, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n");
    fprintf(f, "<html><head><title>404 Not Found</title></head>");
    fprintf(f, "<body><p>The requested path <b>%s</b> was not found.</p></body></html>", path);
}

static int handle_request(int s, http_handler_t handler)
{
	FILE *inf = NULL, *outf = NULL;
	char *method = newstring(300);
	char *path = newstring(300);
	char *line = newstring(300);
	map_t *header = map_create(compare_strings, hash_string);
    map_t *args = map_create(compare_strings, hash_string);
	int t, status = 1;

	/* Create a FILE for reading the request */
	inf = fdopen(s, "r");
	if (inf == NULL) {
		perror("fdopen");
		goto out;
	}
	/* Read and parse the request line */
	if (fscanf(inf, "%300s %300s %*s", method, path) != 2) {
		fprintf(stderr, "Bad request line: %s\n", method);
 		status = 1;
 		goto out;
	}
	if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
		fprintf(stderr, "Unknown method %s\n", method);
		status = 1;
		goto out;
	}
	/* Read the remainder of the request line */
	fgets(line, 300, inf);
	/* Read and parse the header fields */
	while (!feof(inf)) {
		fgets(line, 300, inf);
		if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) {
			/* End of the HTTP header */
			break;
		}
		else {
			/* Parse the name and value of the header field */
			char *name, *value;
            if (splitstring(line, ':', &name, &value)) {
				map_put(header, name, value);
			}
		}
	}

	/* If this is a POST request, read and parse the posted data */
	if (strcmp(method, "POST") == 0) {
		int length;
		char *buf, *p;

		if (!map_haskey(header, "Content-Length")) {
			fprintf(stderr, "No Content-Length in POST request\n");
			goto out;
		}
		length = atoi(map_get(header, "Content-Length"));
		buf = newstring(length+1);
		fread(buf, 1, length, inf);
        buf[length] = '&';
        p = strchr(buf, '&');
        while (p != NULL) {
            char *key, *value;
            *p++ = 0;
            if (splitstring(buf, '=', &key, &value)) {
                key = urldecode(key);
                value = urldecode(value);
                map_put(args, key, value);
            }
            buf = p;
            p = strchr(p, '&');
        }
	}

	/* Create a FILE for writing the response */
	t = dup(s);
	if (t < 0) {
		perror("dup");
		goto out;
	}
	outf = fdopen(t, "w");
	if (outf == NULL) {
		perror("fdopen");
		goto out;
	}
	/* Invoke the request handler to write the response */
	status = handler(path, header, args, outf);

out:
	map_destroy(header);
	map_destroy(args);
	freestrings();
	if (inf != NULL && fclose(inf) > 0) {
		perror("fclose");
		return 1;
	}
	if (outf != NULL && fclose(outf) < 0) {
		perror("fclose");
		return 1;
	}
	return status;
}


int http_server(unsigned short port, http_handler_t handler)
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
   	if (s < 0) {
   		perror("socket");
   		return 1;
   	}
   	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
   		perror("bind");
   		return 1;
   	}
	if (listen(s, 1) < 0) {
		perror("listen");
		return 1;
	}

    /*
     * Accept one TCP connection at a time.
     * (Not how a real web server would do it).
     */
	for (;;) {
		int t, status;
		struct sockaddr_in clientaddr;
		socklen_t len;

        len = sizeof(struct sockaddr_in);
		t = accept(s, (struct sockaddr *) &clientaddr, &len);
		if (t < 0) {
			perror("accept");
			return 1;
		}
		status = handle_request(t, handler);
		if (status != 0) {
			return status;
		}
    }

    /* Never reached */
   	return 0;
}
