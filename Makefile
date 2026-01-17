CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -D_XOPEN_SOURCE=700

TARGET := slosh
SRCS   := slosh.c
OBJS   := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)