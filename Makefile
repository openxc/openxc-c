CC = g++
INCLUDES = -I. -Ideps/cJSON -Ideps/emqueue
CFLAGS = $(INCLUDES) -c -w -Wall -Werror -g -ggdb
LDFLAGS =
LDLIBS = -lcheck -lusb-1.0

TEST_DIR = tests

# Guard against \r\n line endings only in Cygwin
OSTYPE := $(shell uname)
ifneq ($(OSTYPE),Darwin)
	OSTYPE := $(shell uname -o)
	ifeq ($(OSTYPE),Cygwin)
		TEST_SET_OPTS = igncr
	endif
endif

ifdef DEBUG
SYMBOLS += __DEBUG__
else
SYMBOLS += NDEBUG
endif

CFLAGS += $(addprefix -D,$(SYMBOLS))

SRC = $(wildcard openxc/*.c)
SRC += $(wildcard deps/cJSON/cJSON.c)
SRC += $(wildcard deps/emqueue/emqueue.c)
OBJS = $(SRC:.c=.o)
BINARY_SRC = $(wildcard openxc/tools/*.c)
BINARIES = $(BINARY_SRC:.c=.o)
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(TEST_SRC:.c=.o)

all: $(OBJS) openxc/tools/dump

test: $(TEST_DIR)/tests.bin
	@set -o $(TEST_SET_OPTS) >/dev/null 2>&1
	@export SHELLOPTS
	@sh runtests.sh $(TEST_DIR)

openxc/tools/dump: openxc/tools/dump.o $(OBJS)
	$(CC) $(LDFLAGS) $(CC_SYMBOLS) $(INCLUDES) $^ -o $@ $(LDLIBS)

$(TEST_DIR)/tests.bin: $(TEST_OBJS) $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(CC_SYMBOLS) $(INCLUDES) -o $@ $^ $(LDLIBS)


clean:
	rm -rf deps/**/*.o openxc/*.o $(TEST_DIR)/*.o $(TEST_DIR)/*.bin
