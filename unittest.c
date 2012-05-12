#include "unittest.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


unsigned int random_int(void)
{
	unsigned int result = 0;
	FILE *fp = fopen("/dev/random", "r");
	if (fp != NULL) {
		fread(&result, sizeof(result), 1, fp);
		if (ferror(fp) == 0) {
			return result;
		}
	}

	return (unsigned int) time(NULL);
}


int unittest_fail(const char *expr, const char *file, unsigned int line, const char *func)
{
	fprintf(stderr, "%s:%i: %s: Unittest '%s' failed\n", file, line, func, expr);
	return 0;
}
