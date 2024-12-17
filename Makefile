# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -pedantic

# Executable
TARGET = Dungeon_Adventure_Game

# Sources
SRCS = Dungeon_Adventure_Game.c

# Build rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean rule
clean:
	rm -f $(TARGET)
