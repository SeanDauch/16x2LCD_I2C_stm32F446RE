################################################################################
# PROJECT CONFIGURATION
# (You can change these variables for different projects)
################################################################################

# 1. The name of the final executable
TARGET = test

# 2. The specific processor settings for STM32F446
#    -mcpu=cortex-m4:   Defines the processor architecture
#    -mthumb:           Tells GCC to use the Thumb instruction set (required)
#    -mfloat-abi=soft:  Uses software floating point (safer for start)
MCU_SPEC = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft

# 3. Directory for output files (keeps your main folder clean)
BUILD_DIR = build

# 4. Directory for startup files
START_DIR = startup_stm32F446RE
# If you can't find a file in the root folder, look inside these folders too.
VPATH += $(START_DIR)

# 5. Source Files
#    $(wildcard *.c):   Automatically finds ALL .c files in this folder
#    startup_*.s:       Explicitly lists your assembly startup file
C_SOURCES = $(wildcard *.c)
ASM_SOURCES = startup.s

# 6. Linker Script
LDSCRIPT = $(START_DIR)/LinkerScript.ld

################################################################################
# TOOLCHAIN SETTINGS
# (You likely won't need to touch these)
################################################################################

# Define the command line tools
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Compiler Flags
#    -c:              Compile only (don't link yet)
#    -g:              Generate debug info (crucial for debugging!)
#    -Wall:           Turn on all standard warnings (helps find bugs)
#    -O0:             No optimization (best for debugging)
#    --specs=nosys.specs: Fixes "undefined reference to _exit" errors
CFLAGS = $(MCU_SPEC) -g -Wall -O0 --specs=nosys.specs

# Linker Flags
#    -T:              Use our specific Linker Script
#    -Wl,-Map=...:    Generate a .map file (shows memory usage)
LDFLAGS = $(MCU_SPEC) -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nosys.specs

################################################################################
# BUILD RULES
# (The "Magic" that makes it work)
################################################################################

# The list of object files we need to build
# (Converts 'main.c' -> 'build/main.o')
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

# Default rule: What happens when you just type 'make'
all: $(BUILD_DIR)/$(TARGET).bin

# 1. Create the Build Directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $@

# 2. Compile C files -> Object files
#    $@ = The target file (something.o)
#    $< = The source file (something.c)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) -c $(CFLAGS) $< -o $@

# 3. Assemble Assembly files -> Object files
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(CC) -c $(CFLAGS) $< -o $@

# 4. Link everything -> ELF file
#    $^ = All dependency files (all the .o files)
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "Linking..."
	$(CC) $(LDFLAGS) $^ -o $@
	@echo "Build finished. Size:"
	@$(SIZE) $@

# 5. Convert ELF -> Binary (for flashing)
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# --- UTILITIES ---

# Flash the code to the board
flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x8000000

# Delete the build folder
clean:
	rm -rf $(BUILD_DIR)

# Phony targets prevent confusion if you have a file named "clean" or "flash"
.PHONY: all flash clean