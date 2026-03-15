# ========= Toolchain =========
MCU      = atmega328p
CC       = avr-gcc
OBJCOPY  = avr-objcopy

# ========= Default project =========
PROJECT ?= lab01_gpio_timing

# ========= Paths =========
PROJECT_DIR = projects/$(PROJECT)
SRC_DIR     = $(PROJECT_DIR)/src
INC_DIR     = $(PROJECT_DIR)/include
BUILD_DIR   = build/$(PROJECT)

# ========= Files =========
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

ELF = $(BUILD_DIR)/$(PROJECT).elf
HEX = $(BUILD_DIR)/$(PROJECT).hex

# ========= Flags =========
CFLAGS = -mmcu=$(MCU) -Wall -Os -I$(INC_DIR)
LDFLAGS = -mmcu=$(MCU)

# ========= Targets =========
all: $(HEX)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ELF): $(BUILD_DIR) $(OBJ)
	$(CC) $(LDFLAGS) -o $(ELF) $(OBJ)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(HEX)

clean:
	rm -rf build/$(PROJECT)

clean-all:
	rm -rf build

print:
	@echo "PROJECT     = $(PROJECT)"
	@echo "PROJECT_DIR = $(PROJECT_DIR)"
	@echo "SRC         = $(SRC)"
	@echo "OBJ         = $(OBJ)"
	@echo "ELF         = $(ELF)"
	@echo "HEX         = $(HEX)"
