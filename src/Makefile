override THIS_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

override TARGETS := stochar test test-json test-utf8

    all:| $(TARGETS)
  clean:| $(TARGETS:%=clean-%)
install:| $(TARGETS:%=install-%)

override PUBLISH :=            \
      all $(TARGETS)           \
    clean $(TARGETS:%=clean-%) \
  install $(TARGETS:%=install-%)

.PHONY: $(PUBLISH)

# Prevent sub-target tab-completion and direct build.
ifneq (,$(filter $(PUBLISH),$(or $(MAKECMDGOALS),all)))

include $(THIS_DIR)../common.mk

override SRC_stochar := letopt.c stochar.c utf8.c
override DBG_stochar := dbg.c

override SRC_test := cc.c cxx.cpp dstr.c file.c test.c
override DBG_test := dbg.c
override LIBS_test = $(CJSON_LIBS)
override CFLAGS_test.c = $(CJSON_CFLAGS)

override SRC_test-json := json.c test-json.c
override DBG_test-json := dbg.c

override SRC_test-utf8 := letopt.c test-utf8.c utf8.c
override DBG_test-utf8 := dbg.c

$(call pkg_libs,libcjson,CJSON_LIBS,-lcjson)
$(call pkg_cflags,libcjson,CJSON_CFLAGS)
$(call target_rules,$(TARGETS))

all clean install:; $(nop)

endif
