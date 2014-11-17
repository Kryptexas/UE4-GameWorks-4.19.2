TARGET = libdsext.a

CC        = gcc
AR        = ar
RM        = rm -f
MKDIR     = mkdir -p
SED       = sed
FMT       = fmt

SRCDIR    = src
OBJDIR    = obj
BINDIR    = bin
INCDIR    = include
DEPDIR    = deps

CFLAGS    = -std=c11 -Wall -I. -I$(INCDIR) -I../SDL2-2.0.3/include -g

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS     := $(wildcard $(DEPDIR)/*.d)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(MKDIR) $(BINDIR)
	$(AR) rcs $@ $(OBJECTS)

-include $(DEPS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(MKDIR) $(OBJDIR)
	@$(MKDIR) $(DEPDIR)
	$(CC) -c $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) -o $(DEPDIR)/$*.d.tmp $<
	@$(SED) -e 's|.*:|$@:|' < $(DEPDIR)/$*.d.tmp > $(DEPDIR)/$*.d
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $(DEPDIR)/$*.d.tmp | $(FMT) -1 | \
	  $(SED) -e 's/^ *//' -e 's/$$/:/' >> $(DEPDIR)/$*.d
	@$(RM) $(DEPDIR)/$*.d.tmp

.PHONEY: clean

clean:
	$(RM) -r $(OBJDIR)
	$(RM) -r $(DEPDIR)
	$(RM) -r $(BINDIR)
