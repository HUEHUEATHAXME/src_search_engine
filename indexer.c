#include "index.h"
#include "httpd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TIME_ISO "%Y-%m-%d %H:%M:%S"
enum {
	TIME_ISO_LEN = 20,
	DEFAULT_HTTP_PORT = 8080,
};

static char *root;
static index_t *the_index;

static void send_results(FILE *f, char *query, list_t *results)
{
    list_iter_t *it;
    int i = 1;

    fprintf(f, "<hr/><h3>Your query for \"%s\" returned %d result(s)</h3>\n",
            html_escape(query), list_size(results));
    it = list_createiter(results);
    while (list_hasnext(it)) {
        char *path = list_next(it);
        char *htmlpath = html_escape(path);
        fprintf(f, "<p><b>%d.</b> <a href=\"/%s\">%s</a></p>\n",
                i++, htmlpath, htmlpath);
    }
}

static void handle_query(FILE *f, char *query)
{
	time_t now = time(NULL);
	struct tm *tm = gmtime(&now);

	char date_time[TIME_ISO_LEN];
	strftime(date_time, TIME_ISO_LEN, TIME_ISO, tm);
	printf("%s query \"%s\"", date_time, query);

    char *title = "Text Indexer Query Interface";

    http_ok(f, "text/html");
    fprintf(f, "<html><head><title>%s</title></head>\n", title);
    fprintf(f, "<body onLoad=\"queryform.query.focus()\">\n");
    fprintf(f, "<h1>%s</h1>\n", title);
    fprintf(f, "<form name=\"queryform\" action=\".\" method=\"POST\">\n");
    fprintf(f, "<input type=\"text\" name=\"query\" value=\"%s\"/>\n", html_escape(query));
    fprintf(f, "<input type=\"submit\" value=\"Go\"/>\n");
    fprintf(f, "</form>\n");
    if (strcmp(query, "") != 0) {
        char *errmsg;
        list_t *results = index_query(the_index, query, &errmsg);
        if (results == NULL) {
            fprintf(f, "<hr/><h3>Error</h3>\n");
            fprintf(f, "<p>Your query for \"%s\" caused an error: <b>%s</b></p>\n",
                    query, errmsg);

			printf(" -1 \"%s\"", errmsg);
        }
        else {
			printf(" %d", list_size(results));
            send_results(f, query, results);
            list_destroy(results);
        }
    }
    fprintf(f, "</body></html>\n");

	printf("\n");
}

static void handle_page(FILE *f, char *path, char *query)
{
    FILE *pagef = fopen(path, "r");
    if (pagef == NULL) {
        http_notfound(f, path);
    }
    else {
        char buf[1024];
        size_t n;

        http_ok(f, "text/plain");
        while(!feof(pagef)) {
            n = fread(buf, 1, sizeof(buf), pagef);
            fwrite(buf, 1, n, f);
        }
        fclose(pagef);
    }
}

static int http_handler(char *path, map_t *header, map_t *args, FILE *f)
{
    char *query = "";

    if (map_haskey(args, "query")) {
        query = map_get(args, "query");
    }
    if (strcmp(path, "/") == 0) {
        handle_query(f, query);
    }
    else if(path[0] == '/') {
        handle_page(f, path+1, query);
    }
	return 0;
}

void usage_and_die(char *program)
{
	fprintf(stderr, "usage: %s [-p port] <root-dir>\n", program);
	exit(1);
}

int main(int argc, char **argv)
{
    int status = 0;
    list_t *files, *words;
    list_iter_t *it;
    FILE *f;

	int port = DEFAULT_HTTP_PORT;

	char *program = argv[0];
	while (--argc > 0 && **(++argv) == '-') {
		switch((*argv)[1]) {
			case 'p':
				if (--argc > 0) {
					port = atoi(*(++argv));
				} else {
					fprintf(stderr, "option \"-%c\" missing argument\n", (*argv)[1]);
					usage_and_die(program);
				}
				break;
			default:
				fprintf(stderr, "invalid option \"%s\", relevant option character '%c'\n", *argv, (*argv)[1]);
				usage_and_die(program);
				break;
		}
	}

    if (argc <= 0) {
		usage_and_die(program);
    }
    root = *argv;
    files = find_files(root);
    the_index = index_create();

    it = list_createiter(files);
    while (list_hasnext(it)) {
        char *path = list_next(it);
        printf("Indexing %s\n", path);
        f = fopen(path, "r");
        if (f == NULL) {
            perror("fopen");
            fatal_error("fopen() failed");
        }
        words = list_create(compare_strings);
        tokenize_file(f, words);
        index_addpath(the_index, path, words);
        list_destroy(words);
    }
    list_destroyiter(it);
    list_destroy(files);

    printf("Serving queries on port %d\n", port);
	status = http_server(port, http_handler);
    index_destroy(the_index);

    return status;
}
