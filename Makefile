CC := gcc
CFLAGS := -Wall -Werror -Wextra -Wshadow -pedantic -g

TARGET  := http_server

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))


.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ 

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ -c


.PHONY: clean cleanall
clean:
	-rm $(OBJECTS)

cleanall:
	-rm $(TARGET)

