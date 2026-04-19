ifeq ($(origin CC), default)
  CC := clang
endif
CFLAGS += -Wall -Wextra -O2
LDLIBS := -lusb-1.0



all: siren panacea morpheus


siren: companion.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $< $(LOADLIBES) $(LDLIBS) -o $@


panacea morpheus: siren
# remove any old links/files
	rm --force $@
# create hardlink
	ln $< $@


clean:
purge:
	rm -f -- siren panacea morpheus


.PHONY: all
