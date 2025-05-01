ifndef __libcanth_internal_mk_included__
override __libcanth_internal_mk_included__ := 1

override macro_header := $(abspath $(lastword $(MAKEFILE_LIST:internal.mk=cc.h)))

ifneq (,$(filter command file override,$(firstword $(origin DEBUG_MK))))
  ifneq (1,$(strip $(DEBUG_MK)))
    override undefine DEBUG_MK
  endif
else ifneq (undefined,$(origin DEBUG_MK))
  override undefine DEBUG_MK
endif

override undefine w
override w := $w $w
override define n:=


endef
override .      = $n
override .if    = $.\#if
override .elif  = $.\#elif
override .else  = $.\#else
override .ifdef = $.\#ifdef
override .endif = $.\#endif

override pfx-if = $(if $2,$1$2)
override sfx-if = $(if $1,$1$2)
override join-2 = $2$(and $3,$2,$1)$3
override a-an   = a$(if $(filter a% e% i% o% u%,$(word 1,$1)),n) $1
override CSI = $(eval override CSI:=$$(shell printf '\033['))$(CSI)

override desc-origin = $1 is $(call a-an,$(origin $1)) variable
override fail-origin = $(error $(file >/dev/stderr,$(CSI)1;31merror:$(CSI)m $(call desc-origin,$1)))
override warn-origin = $(file >/dev/stderr,$(CSI)1;33mwarning:$(CSI)m $(call desc-origin,$1))

# Read a command line variable with optional fallback.
override arg_var = $(call arg_var_,$(strip $(value 1)),$(strip $(value 2)))

override define arg_bool_var
  $(call arg_var_,$(strip $(value 1)),$(strip $(value 2)))
  $(eval
    ifeq (command line,$$(origin $(strip $1)))
      override $(strip $1):=$$(strip $$($(strip $1)))
      ifneq (1,$$($(strip $1)))
        override undefine $(strip $1)
      endif
    endif
  )
endef

override define arg_var_
  $(eval
    ifeq (environment,$$(origin __default_$1))
      # Unset the __default variable if it originates from the environment
      # (because it's probably not shadowing a file variable).
      $$(call warn-origin,__default_$1)
      $$(if $$(DEBUG_MK),$$(info override undefine __default_$1 [$$(origin __default_$1)]))
      override undefine __default_$1
    else ifeq (,$$(filter file override undefined,$$(firstword $$(origin __default_$1))))
      # Abort if __default might be a command line or environment override
      # variable, as they can be used to mess with the build by clobbering
      # non-override file vars.
      $$(call fail-origin,__default_$1)
    endif

    ifeq (environment,$$(firstword $$(origin $1)))
      # Unset the target variable if it originates from the environment.
      $$(if $$(DEBUG_MK),$$(info override undefine $1 [$$(origin $1)]))
      override undefine $1
    else ifeq (default,$$(origin $1))
      # The target variable is a make builtin default.
      ifdef 2
        # Explicit fallback takes precedence over a builtin default value.
        $$(if $$(DEBUG_MK),$$(info override undefine $1 [default]))
        override undefine $1
      else ifneq (undefined,$$(origin __default_$1))
        # The __default variable takes precedence over the builtin default.
        $$(if $$(DEBUG_MK),$$(info override undefine $1 [$$(origin $1)]))
        override undefine $1
      endif
    else ifneq (undefined,$$(origin $1))
      # The target variable was set in the makefile or on the command line.
      # Unset if it's empty, as we only accept nonempty argument variables
      # from the user. To allow empty command line variables, __default_$1
      # must be defined as empty (not undefined), and there must not be an
      # explicit fallback.
      ifeq (simple,$$(flavor $1))
        ifeq (,$$(strip $$($1)))
          $$(if $$(DEBUG_MK),$$(info override undefine $1 [$$(origin $1)]))
          override undefine $1
        endif
      else ifeq (,$$(strip $$(value $1)))
        $$(if $$(DEBUG_MK),$$(info override undefine $1 [$$(origin $1)]))
        override undefine $1
      endif
    endif

    ifeq (undefined,$$(origin $1))
      # The target variable is undefined, i.e. we can modify it.
      ifdef 2
        $$(if $$(DEBUG_MK),$$(info override $1 = $$$$(eval override $1 := $$(value 2))$$$$($1)))
        override $1 = $$(eval override $1 := $(value 2))$$($1)
      else ifneq (undefined,$$(origin __default_$1))
        $$(if $$(DEBUG_MK),$$(info override $1 = $$$$(eval override $1 := $$(value __default_$1))$$$$($1)))
        override $1 = $$(eval override $1 := $$(value __default_$1))$$($1)
      endif
    endif
  )
endef

# Read a dependency config variable, fall back to pkg-config.
override pkg_libs   = $(call pkg_var,$2,$3,--libs,$1)
override pkg_cflags = $(call pkg_var,$2,$3,--cflags,$1)
override pkg_var    = \
  $(call pkg_var_,$(strip $1),$(strip $2),$(strip $3),$(strip $4))
override define pkg_var_
  $(eval
    ifdef $1
      ifeq (,$$(strip $$($1)))
        override undefine $1
      else ifeq (,$$(filter command,$$(firstword $$(origin $1))))
        override undefine $1
      else
        override __$1$1__ := $$($1)
        override $1 := $$(__$1$1__)
        override undefine __$1$1__
      endif
    endif
    ifndef $1
      $(and $3,$4,override $1 = $$(eval \
        override $1 := $$$$(or $$$$(shell $(PKG_CONFIG) $3 $4),$2))$$($1))
      ifndef $1
        $(if $2,override $1 := $2)
      endif
    endif)
endef

# $1: path of header file to import as make variables
# $2: preprocessor command, defaults to '$(CC) -E -xc'
override dbg-macro-header = $(info $(call .get-macro-header,$(value 1),$(value 2)))

# $1: path of header file to import as make variables
# $2: preprocessor command, defaults to '$(CC) -E -xc'
override get-macro-header = $(eval $(call .get-macro-header,$(value 1),$(value 2)))

# $1: path of header file to import as make variables
# $2: preprocessor command, defaults to '$(CC) -E -xc'
override .get-macro-header = $(shell $(or $2,$(CC) -E -xc) -w -P $1)

override define import-macros
$(foreach v,$(3:%=$(strip $1)%) $4,$(eval override $v = $$(.get-$(strip $1)-$(macro_header))$$($v)))
$(eval override .get-$(strip $1)-$(macro_header) = $$(eval \
  $$(call get-macro-header,$(macro_header),$2)override undefine .get-$(strip $1)-$(macro_header)))
endef

override about = $(if $(strip $2),$(call .about,$(if            \
                 $(strip $(CC_OBJ_$(strip $1))),$(CC_id)),$(if  \
                 $(strip $(CXX_OBJ_$(strip $1))),$(CXX_id))))

# Generate rules and dependencies.
override define target_rules
$(eval override .O=$$(eval override .O:=$$$$(shell bash -c 'a="$$$$O";  \
  b=$$$$$$$$(realpath --relative-to="$$$$(THIS_DIR)" "$$$$$$$$a");      \
  (( $$$$$$$${#a} > $$$$$$$${#b} )) || b="$$$$$$$${a%/}"; printf "%s/"  \
  "$$$$$$$$b"'))$$(if $$(.O:./=),,$$(eval override .O:=))$$(.O)$n       \
  override undefine OBJ                                                 \
)$(foreach t,$1,$(eval $t:| $$O$t)                                      \
  $(eval override DBG_OBJ_$t := $$(DBG_$t:%=$$O%.o))                    \
  $(eval override DBG_DEP_$t := $$(DBG_OBJ_$t:%.o=%.d))                 \
  $(eval override OBJ_$t     := $$(SRC_$t:%=$$O%.o)                     \
                                $$(if $$(debug),$$(DBG_OBJ_$t)))        \
  $(eval override DEP_$t     := $$(OBJ_$t:%.o=%.d))                     \
  $(eval override CC_OBJ_$t  := $$(OBJ_$t:%.cpp.o=))                    \
  $(eval override CXX_OBJ_$t := $$(OBJ_$t:%.c.o=))                      \
  $(eval $$O$t: $$(OBJ_$t);                                             \
    $$(call about,$t,$$?)                                               \
    $$(call msg,LINK,$$(.O)$t)                                          \
    +$$Q$$(CC) $$(C_BUILDFLAGS) $$(strip                                \
      $$(CFLAGS_$t) -o $$@ $$^ $$(LIBS_$t)))                            \
  $(eval $$(foreach s,$$(SRC_$t)$$(if $$(debug), $$(DBG_$t)),           \
    $$(eval $$O$$s.o: $$(THIS_DIR)$$s)))                                \
  $(if $(filter-out $(OBJ),$(CC_OBJ_$t)),                               \
    $(eval $$(filter-out $$(OBJ),$$(CC_OBJ_$t)):;                       \
      $$(call about,$t,$$?)                                             \
      $$(call msg,CC,$$(@:$$O%=$$(.O)%))                                \
      +$$Q$$(CC) $$(C_BUILDFLAGS) $$(strip                              \
        $$(CPPFLAGS_$$(<:$$(THIS_DIR)%=%)) $$(CPPFLAGS_$t)              \
        $$(CPPFLAGS_$t_$$(<:$$(THIS_DIR)%=%))                           \
        $$(CFLAGS_$$(<:$$(THIS_DIR)%=%)) $$(CFLAGS_$t)                  \
        $$(CFLAGS_$t_$$(<:$$(THIS_DIR)%=%)) -c -o $$@ -MMD $$<)))       \
  $(if $(filter-out $(OBJ),$(CXX_OBJ_$t)),                              \
    $(eval $$(filter-out $$(OBJ),$$(CXX_OBJ_$t)):;                      \
      $$(call about,$t,$$?)                                             \
      $$(call msg,CXX,$$(@:$$O%=$$(.O)%))                               \
      +$$Q$$(CXX) $$(CXX_BUILDFLAGS) $$(strip                           \
        $$(CPPFLAGS_$$(<:$$(THIS_DIR)%=%)) $$(CPPFLAGS_$t)              \
        $$(CPPFLAGS_$t_$$(<:$$(THIS_DIR)%=%))                           \
        $$(CXXFLAGS_$$(<:$$(THIS_DIR)%=%)) $$(CXXFLAGS_$t)              \
        $$(CXXFLAGS_$t_$$(<:$$(THIS_DIR)%=%)) -c -o $$@ -MMD $$<)))     \
  $(eval clean-$t: $$(eval override WHAT_$t := $$$$(sort $$$$(wildcard  \
    $$O$t $$(OBJ_$t) $$(DBG_OBJ_$t) $$(DEP_$t) $$(DBG_DEP_$t)           \
    $$(addprefix $$O*.,args.0 bc i ii lto_wrapper_args ltrans_args      \
      ltrans0.o ltrans.o ltrans.out res s temp.o)))))                   \
  $(eval clean-$t: $$(if $$(WHAT_$t),| yeet);$$(if $$(WHAT_$t),         \
    $$(call msg,CLEAN,$$(.O)$t)$$Q$$(RM) $$(WHAT_$t),$(nop)))           \
  $(eval install-$t:; $$(call msg,INSTALL,$$(.O)$t)$(nop))              \
  $(eval override OBJ := $$(sort $$(OBJ) $$(OBJ_$t)))                   \
)$(eval                                                                 \
  override DEP := $$(OBJ:.o=.d)$n                                       \
  -include $$(DEP)                                                      \
)
endef

endif
