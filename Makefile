CC = gcc
CFLAGS = -O2 -Wall -Wextra

SRV_SRCDIR = rkvs-srv
SRV_BUILDDIR = build
CLI_SRCDIR = rkvs-cli
CLI_BUILDDIR = build

SRV = $(SRV_BUILDDIR)/rkvs
LIBCLI = $(CLI_BUILDDIR)/libcli

SRV_C_SOURCES = $(shell find $(SRV_SRCDIR)/ -type f -name "*.c")
CLI_C_SOURCES = $(shell find $(CLI_SRCDIR)/ -type f -name "*.c")

all: clean $(SRV) $(LIBCLI)

$(SRV): $(SRV_C_SOURCES)
	@echo "----- Building rkvs server -----"
	@mkdir -p $(SRV_BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(LIBCLI): $(CLI_C_SOURCES)
	@echo "----- Building rkvs libclient -----"
	@mkdir -p $(CLI_BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

clean: 
	@echo "----- Cleaning -----"
	rm -rf $(SRV) $(LIBCLI)

run: all
	@echo "----- Running rkvs server -----"
	./$(SRV) 8282

.PHONY: all run  clean

