-include config.mk

CC ?= gcc
CFLAGS := -Wall -Wextra -O2
PREFIX ?= /usr/local
CONFIG_DIR ?= /etc/ai
TARGET := wrapped-ai

build: $(TARGET)

$(TARGET): ai-cage.c cJSON.c cJSON.h
	$(CC) $(CFLAGS) -o $@ ai-cage.c cJSON.c -lm

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

install-config:
	install -d $(DESTDIR)$(CONFIG_DIR)
	install -m 644 config.json $(DESTDIR)$(CONFIG_DIR)/config.json

clean:
	rm -f $(TARGET)

.PHONY: build install install-config clean
an
