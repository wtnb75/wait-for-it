
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

clean:
	rm -f $(PROGRAM) cover.out cover.html

test: $(SOURCES)
	GO111MODULE=auto $(GO) test $(TEST_GOFLAGS)

coverage: test
	go tool cover -html=cover.out -o cover.html
