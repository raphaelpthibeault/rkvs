CC = gcc
CFLAGS = -O2 -Wall -Wextra -I./

SRV_SRCDIR = rkvs-srv
SRV_BUILDDIR = build-srv
CLI_SRCDIR = rkvs-cli
CLI_BUILDDIR = build-cli
COMMON_SRCDIR = common

SRV = $(SRV_BUILDDIR)/rkvs
LIBCLI = $(CLI_BUILDDIR)/libcli

COMMON_C_SOURCES = $(shell find $(COMMON_SRCDIR)/ -type f -name "*.c")
SRV_C_SOURCES = $(shell find $(SRV_SRCDIR)/ -type f -name "*.c") $(COMMON_C_SOURCES)
CLI_C_SOURCES = $(shell find $(CLI_SRCDIR)/ -type f -name "*.c") $(COMMON_C_SOURCES)

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
	rm -rf $(SRV_BUILDDIR) $(CLI_BUILDDIR)

run: all
	@echo "----- Running rkvs server -----"
	./$(SRV) 8282

.PHONY: all run clean

