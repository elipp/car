CC=gcc -g
CFLAGS=-c -Wall

# If the symbol NO_GNU_READLINE is defined, the replacement library (ANSIX3_64_readline_emul)
# will be used instead. You should, however, prefer the thoroughly tested GNU Readline library over this.

#CONFIG=-DLONG_DOUBLE_PRECISION 
CONFIG=-DLONG_DOUBLE_PRECISION -DNO_GNU_READLINE -DUSE_CHEM_PLUGINS -DC99_AVAILABLE
#CONFIG=-DNO_GNU_READLINE -DUSE_CHEM_PLUGINS

SOURCES=calc.c commands.c tree.c functions.c ud_constants_tree.c tables.c utils.c rl_emul.c chem/chem.c chem/atomic_weights.c
OBJS=tree.o commands.o functions.o tables.o ud_constants_tree.o utils.o rl_emul.o chem.o atomic_weights.o
OBJDIR=objs
SRCDIR=src
objects = $(addprefix $(OBJDIR)/, $(OBJS))

EXECUTABLES=calc result_test tewls/print_raw

all: objdir $(EXECUTABLES)

calc: $(objects)
	$(CC) $(CONFIG) $(LIBS) -lm $(objects) $(SRCDIR)/calc.c -o calc

result_test: $(objects)
	$(CC) $(CONFIG) -lm $(objects) src/test/result_test.c -o result_test 


# linux (well, bash) specific
objdir:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

$(OBJDIR)/commands.o: $(SRCDIR)/commands.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/ud_constants_tree.o: $(SRCDIR)/ud_constants_tree.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/tree.o: $(SRCDIR)/tree.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/functions.o: $(SRCDIR)/functions.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/tables.o: $(SRCDIR)/tables.c
	$(CC) $(CONFIG) -lm $(CFLAGS) $< -o $@

$(OBJDIR)/utils.o: $(SRCDIR)/utils.c
	$(CC) $(CONFIG) -lm $(CFLAGS) $< -o $@

$(OBJDIR)/rl_emul.o: $(SRCDIR)/rl_emul.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/chem.o: $(SRCDIR)/chem/chem.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

$(OBJDIR)/atomic_weights.o: $(SRCDIR)/chem/atomic_weights.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@

tewls: $(OBJDIR)/tewls.o
	$(CC) $< -o tewls/print_raw
		
$(OBJDIR)/tewls.o: 
	$(CC) $(CFLAGS) tewls/print_raw.c -o $@
	

clean:
	rm -rf $(EXECUTABLES) $(OBJDIR)/*.o


