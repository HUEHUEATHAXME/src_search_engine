CC=gcc
CFLAGS=-Wall -g -pg

COMMON_SRC=common.c
LIST_SRC=linkedlist.c
SET_SRC=aatreeset.c $(LIST_SRC)
MAP_SRC=hashmap.c $(SET_SRC)
QUERY_PARSER_SRC=query_parser.c $(COMMON_SRC) $(LIST_SRC) $(MAP_SRC) $(SET_SRC)
INDEX_SRC=index.c $(QUERY_PARSER_SRC) $(COMMON_SRC) $(LIST_SRC) $(MAP_SRC) $(SET_SRC)
INDEXER_SRC=indexer.c httpd.c $(COMMON_SRC) $(LIST_SRC) $(MAP_SRC) $(SET_SRC) $(INDEX_SRC)
HEADERS=common.h httpd.h list.h set.h map.h index.h
UNITTEST=unittest.c

all: indexer

indexer: $(INDEXER_SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *~ *.o *.exe indexer *.test *.test.exe

.PHONY: test
test: index.test
	for i in $^; do echo $$i:; ./$$i 2>&1; done

INDEX_TEST_SRC=index.test.c $(UNITTEST) $(INDEX_SRC) $(COMMON_SRC) $(LIST_SRC)

index.test: $(INDEX_TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^
