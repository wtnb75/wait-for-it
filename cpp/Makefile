
CC = gcc
CXX = g++
CFLAGS = -Wall
CXXFLAGS = -Wall -std=c++11
LDFLAGS =
LIBS = -lgflags
PROGRAM = wait-for-it
OBJS = wait-for-it.o
TEST_CFLAGS = -fprofile-arcs -ftest-coverage -Wall -std=c++11
TEST_LIBS = -lcppunit -lgflags
PROGRAM_static = wait-for-it.static
LIBS_static =
LDFLAGS_static =

all: $(PROGRAM)

static: $(PROGRAM_static)

$(PROGRAM): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGRAM)

$(PROGRAM_static): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS_static) $(LIBS_static) -static -o $(PROGRAM_static)

clean:
	rm -f $(OBJS) $(PROGRAM) test
	rm -f *.gcno *.gcov *.gcda *.gch lcov.info
	rm -rf cover

test: wait-for-it.cpp test_waitfor.cpp
	$(CXX) test_waitfor.cpp $(TEST_CFLAGS) $(TEST_LIBDIR) $(TEST_LIBS) -o $@

check: test
	@./test

coverage: check
	gcov test_waitfor.gcda -l

coverage-html: coverage
	lcov -c -d . -o lcov.info --no-external
	genhtml lcov.info -o cover
