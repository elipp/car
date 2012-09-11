CC=gcc -g
CFLAGS=-c -Wall

# If the symbol NO_GNU_READLINE is defined, the replacement library (vt100_readline_emul)
# will be used instead. You should, however, prefer the thoroughly tested GNU Readline library over this.

CONFIG=-DLONG_DOUBLE_PRECISION -lreadline
#CONFIG=-DLONG_DOUBLE_PRECISION -DNO_GNU_READLINE

SOURCES=calc.c commands.c tree.c functions.c ud_constants_tree.c tables.c utils.c vt100_readline_emul.c
OBJS=tree.o commands.o functions.o tables.o ud_constants_tree.o utils.o vt100_readline_emul.o
OBJDIR=objs
SRCDIR=src
objects = $(addprefix $(OBJDIR)/, $(OBJS))

EXECUTABLES=calc result_test

all: $(EXECUTABLES)

calc: $(objects)
	$(CC) $(CONFIG) $(LIBS) -lm $(objects) $(SRCDIR)/calc.c -o calc

result_test: $(objects)
	$(CC) $(CONFIG) -lm $(objects) src/test/result_test.c -o result_test 



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

$(OBJDIR)/vt100_readline_emul.o: $(SRCDIR)/vt100_readline_emul.c
	$(CC) $(CONFIG) $(CFLAGS) $< -o $@



clean:
	rm -rf $(EXECUTABLES) $(OBJDIR)/*.o
