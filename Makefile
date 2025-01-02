CC = gcc
CPP = g++
AS = nasm
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = 
ASMFLAGS = -f bin

SRC_DIR = C
OBJ_DIR = obj
BIN_DIR = bin

C_SRCS = $(filter-out $(wildcard $(SRC_DIR)/*.c), $(shell find $(SRC_DIR) -type f -size +0c -name '*.c'))
ASM_SRCS = $(filter-out $(wildcard $(SRC_DIR)/*.s), $(shell find $(SRC_DIR) -type f -size +0c -name '*.s'))
C_BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(notdir $(C_SRCS:.c=)))
ASM_BINS = $(patsubst $(SRC_DIR)/%.s, $(BIN_DIR)/%.bin, $(notdir $(ASM_SRCS:.s=)))

.PHONY: all clean chroot

all: $(C_BINS) $(ASM_BINS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/%.bin: $(SRC_DIR)/%.s
	@mkdir -p $(BIN_DIR)
	$(AS) $(ASMFLAGS) -o $@ $<

chroot: $(BIN_DIR)/migux_chroot
	sudo chroot $(BIN_DIR) /migux_chroot

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)