SCRIPT_NAME=script
SYN_BUILD_DIR=linux
SYN_NAME=syn

BASEDIR=..
BLDDIR=.
ODIR=$(BLDDIR)/obj
EDIR=$(BLDDIR)/bin

SCRIPT_EXE=$(EDIR)/$(SCRIPT_NAME)
SYN_BLDDIR=$(BASEDIR)/../syn/$(SYN_BUILD_DIR)
SYN_EXE=$(SYN_BLDDIR)/bin/$(SYN_NAME)

DEPS=

ifeq (1,$(DEBUG))
DEBUG_FLAGS=-Og -fdebug-cpp --debug -ggdb -D_DEBUG -DDEBUG
else
DEBUG_FLAGS=-O3 -DNDEBUG
endif

CFLAGS=-std=c++11 -static-libgcc -static-libstdc++ -Wall $(DEBUG_FLAGS)
CC=g++

script: mkdirs $(SCRIPT_EXE)

mkdirs:
	"mkdir" -p $(EDIR)
	"mkdir" -p $(ODIR)

$(ODIR)/%.o: $(BASEDIR)/core/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)/core -I$(BASEDIR)/../syn/rt -I$(ODIR)

_OBJ = api.o api_basic.o api_collection.o api_execute.o api_file.o api_io.o api_socket.o ast_declaration.o ast_expression.o ast_script.o \
ast_statement.o ast_type.o basetype.o common.o syngen.o gc.o gc_hashmap.o gc_vector.o main.o name.o op.o platform_file_linux.o \
platform_file_common.o platform_linux.o platform_socket_common.o sample.o scanner.o scope.o script.o stacktrace.o stringex.o sysclass.o \
sysclassbld.o sysvalue.o value.o value_core.o value_util.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ)) $(ODIR)/syngen.o $(SYN_BLDDIR)/obj/rt/syn.o

$(ODIR)/syngen.o: $(ODIR)/syngen.cpp $(ODIR)/syngen.h
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)/../syn/rt -I$(BASEDIR)/core

$(ODIR)/main.o: $(BASEDIR)/main/main.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/syngen.cpp $(ODIR)/syngen.h: $(SYN_EXE) $(BASEDIR)/core/grammar.txt
	$(SYN_EXE) -i ast_combined.h -mm "syn_^" -n syn_script::ast -ng syn_script::syngen -a syn_script::ast::AstAllocator -s \
$(BASEDIR)/core/grammar.txt $(ODIR)/syngen

$(SCRIPT_EXE): $(OBJ)
	$(CC) -o $@ $^ -lm

clean:
	"rm" -f -r $(ODIR)
	"rm" -f -r $(EDIR)
