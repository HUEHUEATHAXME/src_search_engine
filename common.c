#include "common.h"
#include "list.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

void fatal_error(char *msg)
{
    fprintf(stderr, "fatal error: %s\n", msg);
    exit(1);
}

void tokenize_file(FILE *file, list_t *list)
{
    char *word;
    char buf[101];
    buf[100] = 0;

    while (!feof(file)) {
		/* Skip non-letters */
		fscanf(file, "%*[^a-zA-Z0-9'_]");
		/* Scan up to 100 letters */
		if (fscanf(file, "%100[a-zA-Z0-9'_]", buf) == 1) {
	    	word = strdup(buf);
	    	if (word == NULL)
				fatal_error("out of memory");
	    	list_addlast(list, word);
		}
    }
}

enum { OK_FILE_TYPE = (S_IFREG | S_IFDIR) };
int _read_dir(char *root, list_t *list)
{
	if (root == NULL || list == NULL) {
		return 1;
	}

	DIR *dp = opendir(root);
	if (dp == NULL) {
		perror(root);
		goto error;
	}

	struct dirent *file;
	while((file = readdir(dp)) != NULL) {
		if (file->d_name[0] == '.') {
			/* skip hidden files and ".", "..", so we do up in the hierarchy */
			continue;
		}

		int file_path_len = strlen(root)+1+strlen(file->d_name)+1; // +1+ for slash +1 for '\0'
		char *file_path = (char *)malloc(sizeof(*file_path)*file_path_len);
		if (file_path == NULL) {
			perror("malloc");
			goto error;
		}

		snprintf(file_path, file_path_len, "%s/%s", root, file->d_name);

		struct stat s;
		if (!(stat(file_path, &s) == 0 && (s.st_mode & OK_FILE_TYPE))) {
			fprintf(stderr, "skipping irregular file '%s'\n", file_path);
			free(file_path);
		} else if ((s.st_mode & S_IFDIR)) {
			_read_dir(file_path, list);
		} else {
			FILE *fp = fopen(file_path, "r");
			if (fp == NULL) {
				perror(file_path);
				free(file_path);
			} else {
				list_addlast(list, file_path);
				fclose(fp);
			}
		}
	}

	closedir(dp);
	return 0;

error:
	if (dp != NULL) {
		closedir(dp);
	}
	return 1;
}

struct list *find_files(char *root)
{
    list_t *files = list_create(compare_strings);
	if (files == NULL) {
		return NULL;
	}

	if (_read_dir(root, files) > 0) {
		list_destroy(files);
		return NULL;
	} else {
		return files;
	}
}

int compare_strings(void *a, void *b)
{
    return strcmp(a, b);
}

unsigned long hash_string(void *str)
{
    unsigned char *p = str;
    unsigned long hash = 5381;
    int c;

    while ((c = *p++) != '\0')
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

int compare_pointers(void *a, void *b)
{
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	return 0;
}
