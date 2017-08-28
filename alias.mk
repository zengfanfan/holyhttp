define PRINT_COLOR
	printf "\033[1;40;32m  %8s \033[1;40;36m %s \033[0m\r\n" $(1) $(2)
endef

define _NOP_
	@echo $$* >> /dev/null
endef

_O_CC_ := $(CC)
define CC
	@$(call PRINT_COLOR, CC, $@)
	@$(_O_CC_)
endef

_O_LD_ := $(LD)
define LD
	@$(call PRINT_COLOR, LD, $@)
	@$(_O_LD_)
endef

_O_AR_ := $(AR)
define AR
	@$(call PRINT_COLOR, AR, $@)
	@$(_O_AR_)
endef

_O_AS_ := $(AS)
define AS
	@$(call PRINT_COLOR, AS, $@)
	@$(_O_AS_)
endef

_O_STRIP_ := $(STRIP)
ifeq ($(_DEBUG_),y)
	STRIP := $(_NOP_)
else ifeq ($(_O_STRIP_),)
	STRIP := $(_NOP_)
else
define STRIP
	@$(call PRINT_COLOR, STRIP, $@)
	@$(_O_STRIP_)
endef
endif

_O_RM_ := $(RM)
define RM
	@-_my_func_() \
	{\
		$(call PRINT_COLOR, RM, "$$*");\
		$(_O_RM_) $$*;\
	};\
	_my_func_
endef

define INSTALL
	@-_my_func_() \
	{\
		$(call PRINT_COLOR, INSTALL, $$1);\
		cp -f $$1 $$2;\
	};\
	_my_func_
endef

define ENTER
	@-_my_func_() \
	{\
		$(call PRINT_COLOR, '..', $$*);\
	};\
	_my_func_
endef

define LEAVE
	@-_my_func_() \
	{\
		$(call PRINT_COLOR, '..', $$*);\
	};\
	_my_func_
endef

