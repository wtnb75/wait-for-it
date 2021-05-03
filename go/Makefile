
GO = go
GOFLAGS = -ldflags="-s -w"
LDFLAGS =
LIBS =
PROGRAM = wait-for-it
SOURCES = wait-for-it.go
TEST_GOFLAGS = -coverprofile=cover.out

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(GO) build $(GOFLAGS) $(LDFLAGS) $(LIBS) -o $(PROGRAM) $(SOURCES)

dep:
	$(GO) mod download

clean:
	rm -f $(PROGRAM) cover.out cover.html

test: $(SOURCES)
	$(GO) test $(TEST_GOFLAGS)

coverage: test
	sed -e 's,wait-for-it.go/wait-for-it.go,./wait-for-it.go,g;' cover.out > cover.out2
	go tool cover -html=cover.out2 -o cover.html
