CC := cc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c11
LDFLAGS :=

TARGET := ChDir
SRC := ChDir.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
