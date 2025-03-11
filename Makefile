CC = gcc
CFLAGS = -Isrc -Wall -Wextra -O2 -DNDEBUG
CFLAGS_DEBUG := -Isrc -Wall -Wextra -g -D_DEBUG
LDFLAGS = -lSDL2 -lm
SRC := $(shell find src -type f -name '*.c')

OBJ_DIR_DEBUG := bin/obj/debug
OBJ_DIR_RELEASE := bin/obj/release

OBJ_DEBUG := $(patsubst src/%, $(OBJ_DIR_DEBUG)/%, $(SRC:.c=.o))
OBJ_RELEASE := $(patsubst src/%, $(OBJ_DIR_RELEASE)/%, $(SRC:.c=.o))

# Compilação Release
bin/release: $(OBJ_RELEASE)
	$(CC) $(OBJ_RELEASE) -o $@ $(LDFLAGS)

$(OBJ_DIR_RELEASE)/%.o: src/%.c | $(OBJ_DIR_RELEASE)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

bin/debug: $(OBJ_DEBUG)
	$(CC) $(OBJ_DEBUG) -o $@ $(LDFLAGS)

$(OBJ_DIR_DEBUG)/%.o: src/%.c | $(OBJ_DIR_DEBUG)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS_DEBUG)

# Criar diretórios
$(OBJ_DIR_DEBUG) $(OBJ_DIR_RELEASE):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf bin/obj bin/debug bin/release
