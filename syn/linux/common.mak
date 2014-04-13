BASEDIR=..
BLDDIR=.
ODIR=$(BLDDIR)/obj
EDIR=$(BLDDIR)/bin

SYN_EXE=$(EDIR)/$(SYN_NAME)
TEST_EXE=$(EDIR)/$(TEST_NAME)

DEPS=

ifeq (1,$(DEBUG))
DEBUG_FLAGS=-fdebug-cpp -Og --debug -ggdb -D_DEBUG -DDEBUG
else
DEBUG_FLAGS=-DNDEBUG
endif

CFLAGS=-std=c++11 -static-libgcc -static-libstdc++ $(DEBUG_FLAGS)
CC=g++

syn: mkdirs $(SYN_EXE)
test: mkdirs $(TEST_EXE)

#
#mkdirs
#


mkdirs:
	"mkdir" -p $(EDIR)
	"mkdir" -p $(ODIR)/core
	"mkdir" -p $(ODIR)/rt
	"mkdir" -p $(ODIR)/start
	"mkdir" -p $(ODIR)/test

#
#SYN_EXE
#


$(ODIR)/core/%.o: $(BASEDIR)/core/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)/rt

$(ODIR)/rt/%.o: $(BASEDIR)/rt/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)/rt

$(ODIR)/start/%.o: $(BASEDIR)/start/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)/rt

_OBJ = action.o action_factory.o cmdline.o codegen.o codegen_action.o commons.o concretelrgen.o concretescan.o conversion.o \
conversion_builder.o converter.o descriptor.o descriptor_type.o ebnf.o ebnf_bld_attrs.o ebnf_bld_gentype.o ebnf_bld_name.o ebnf_bld_recursion.o \
ebnf_bld_type.o ebnf_bld_void.o ebnf_builder.o ebnf_extension.o grm_parser.o grm_scanner.o main.o types.o util_string.o

OBJ = $(patsubst %,$(ODIR)/core/%,$(_OBJ)) $(ODIR)/rt/syn.o $(ODIR)/start/start.o

$(SYN_EXE): $(OBJ)
	$(CC) -o $@ $^ -lm

#
#TEST_EXE
#

$(ODIR)/test/%.o: $(BASEDIR)/test/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS) -I$(BASEDIR)

_TEST_OBJ = cmdline_test.o converter_test.o ebnf_bld_attrs_test.o ebnf_bld_gentype_test.o ebnf_bld_recursion_test.o \
ebnf_bld_type_test.o ebnf_bld_void_test.o ebnf_builder_test.o grm_parser_test.o raw_bnf_test.o tests.o unittest.o \
util_string_test.o
TEST_OBJ = $(patsubst %,$(ODIR)/core/%,$(_OBJ)) $(patsubst %,$(ODIR)/test/%,$(_TEST_OBJ)) $(ODIR)/rt/syn.o 

$(TEST_EXE): $(TEST_OBJ)
	$(CC) -o $@ $^ -lm

#
#clean
#

clean:
	"rm" -f -r $(ODIR)
	"rm" -f -r $(EDIR)
