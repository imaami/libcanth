override THIS_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

override TARGETS := test

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

override SRC_test := cc.c cxx.cpp dstr.c file.c test.c
override DBG_test := dbg.c
override LIBS_test = $(CJSON_LIBS)
override CFLAGS_test.c = $(CJSON_CFLAGS)

$(call pkg_libs,libcjson,CJSON_LIBS,-lcjson)
$(call pkg_cflags,libcjson,CJSON_CFLAGS)
$(call target_rules,$(TARGETS))

all clean install:; $(nop)

endif
