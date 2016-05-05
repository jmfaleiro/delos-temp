CFLAGS=-O2 -g -Wall -Wextra -Werror -std=c++0x -w
CXX=g++

INCLUDE=include
SRC=src
SOURCES:=$(wildcard $(SRC)/*.cc $(SRC)/*.c)
OBJECTS:=$(patsubst $(SRC)/%.cc,build/%.o,$(SOURCES))

DEPSDIR:=.deps
DEPCFLAGS=-MD -MF $(DEPSDIR)/$*.d -MP

all:build/client

-include $(wildcard $(DEPSDIR)/*.d)

build/%.o: src/%.cc $(DEPSDIR)/stamp GNUmakefile
	@mkdir -p build
	@echo + cc $<
	@$(CXX) $(CFLAGS) $(DEPCFLAGS) -I$(INCLUDE) -c -o $@ $<

build/client: $(OBJECTS)
	@$(CXX) $(CFLAGS) -o $@ $^ -L. libc_link.a

$(DEPSDIR)/stamp:
	@mkdir -p $(DEPSDIR)
	@touch $@

.PHONY: clean

clean:
	rm -rf build $(DEPSDIR) $(TESTOBJECTS) start/*.o
