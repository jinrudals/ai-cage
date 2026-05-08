bin_PROGRAMS = ai-cage

COMMON_SOURCES = src/main.c src/backend.h cJSON.c cJSON.h

if BACKEND_LANDLOCK
ai_cage_SOURCES = $(COMMON_SOURCES) src/backend_landlock.c
ai_cage_CFLAGS = -Wall -Wextra -O2 -DCONFIG_DIR=\"$(CONFIG_DIR)\"
else
ai_cage_SOURCES = $(COMMON_SOURCES) src/backend_apptainer.c
ai_cage_CFLAGS = -Wall -Wextra -O2 -DCONFIG_DIR=\"$(CONFIG_DIR)\" \
    -DAPPTAINER_BIN=\"$(APPTAINER_BIN)\" \
    -DAPPTAINER_IMAGES=\"$(APPTAINER_IMAGES)\"
endif

ai_cage_LDADD = -lm

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(CONFIG_DIR)
	$(INSTALL_DATA) config.json $(DESTDIR)$(CONFIG_DIR)/config.json
