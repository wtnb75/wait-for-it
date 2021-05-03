
CC = gcc
CFLAGS = -Wall
LDFLAGS =
LIBS =
PROGRAM = wait-for-it
OBJS = wait-for-it.o
TEST_CFLAGS = -fprofile-arcs -ftest-coverage
TEST_LIBS = -lcunit
PROGRAM_static = wait-for-it.static
LIBS_static =
LDFLAGS_static =

all: $(PROGRAM)

static: $(PROGRAM_static)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGRAM)

$(PROGRAM_static): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_static) $(LIBS_static) -static -o $(PROGRAM_static)

clean:
	rm -f $(OBJS) $(PROGRAM) test
	rm -f *.gcno *.gcov *.gcda *.gch lcov.info
	rm -rf cover

test: wait-for-it.c test_waitfor.c
	$(CC) -g test_waitfor.c $(TEST_CFLAGS) $(TEST_LIBDIR) $(TEST_LIBS) -o $@

check: test
	@./test

coverage: check
	gcov test_waitfor.gcda -l

coverage-html: coverage
	lcov -c -d . -o lcov.info --no-external
	genhtml lcov.info -o cover
