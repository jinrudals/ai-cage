bin_PROGRAMS = ai-cage
ai_cage_SOURCES = ai-cage.c cJSON.c cJSON.h
ai_cage_CFLAGS = -Wall -Wextra -O2 -DCONFIG_DIR=\"$(CONFIG_DIR)\"
ai_cage_LDADD = -lm

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(CONFIG_DIR)
	$(INSTALL_DATA) config.json $(DESTDIR)$(CONFIG_DIR)/config.json
