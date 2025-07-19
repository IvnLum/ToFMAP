# Compiler set, executable name, compile flags
CC := g++
EXID := "run"
CFLAGS := -Wall -std=c++23
CLINKERS := -lm -lglut -lGLU -lGL -latomic

# Header files, source files, build output dir, executable output dir

HDRDIR := include
HDREXT := hpp

SRCDIR := src
SRCEXT := cpp

BUILDDIR := build

EXECDIR := bin

#
#

TARGET := $(EXECDIR)/$(EXID)
SOURCES := $(shell find $(SRCDIR) $(HDRDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/main/%,$(SOURCES:.$(SRCEXT)=.o))
INC := -I $(HDRDIR)

#
#

all: main 

main: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linker <- main"
	@echo " $(CC) $^ -o $(TARGET)"; $(CC) $^ -o $(TARGET) $(CLINKERS)

-include $(OBJECTS:.o=.d)

$(BUILDDIR)/main/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(EXECDIR)
	@mkdir -p $(BUILDDIR)/main
	@mkdir -p $(@D)
	@echo " $(CC) -MM -MT"$@" $(CFLAGS) $(INC) $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/main/$*.d"; $(CC) -MM -MT"$@" $(CFLAGS) $(INC) $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/main/$*.d
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(EXECDIR) $(BUILDDIR) $(TARGET) $(TESTTARGET) $(HDRDIR)/*.o"; $(RM) -r $(EXECDIR) $(BUILDDIR) $(TARGET) $(TESTTARGET) $(HDRDIR)/*.o


.PHONY: all, main, tests, clean
