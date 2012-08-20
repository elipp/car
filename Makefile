CC=gcc -O2 
CFLAGS=-c -Wall

# change this at will, default is long double
DEFINES=-DLONG_DOUBLE_PRECISION
#DEFINES=

LIBS=-lreadline
SOURCES=calc.c tree.c functions.c tables.c utils.c
OBJS=tree.o functions.o tables.o utils.o
OBJDIR=objs
SRCDIR=src
objects = $(addprefix $(OBJDIR)/, $(OBJS))

EXECUTABLE=calc

all: calc

calc: $(objects)
	$(CC) $(DEFINES) $(LIBS) -lm $(objects) $(SRCDIR)/calc.c -o $(EXECUTABLE)

$(OBJDIR)/tree.o: $(SRCDIR)/tree.c
	$(CC) $(DEFINES) $(CFLAGS) $< -o $@

$(OBJDIR)/functions.o: $(SRCDIR)/functions.c
	$(CC) $(DEFINES) $(CFLAGS) $< -o $@

$(OBJDIR)/tables.o: $(SRCDIR)/tables.c
	$(CC) $(DEFINES) -lm $(CFLAGS) $< -o $@

$(OBJDIR)/utils.o: $(SRCDIR)/utils.c
	$(CC) $(DEFINES) -lm $(CFLAGS) $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJDIR)/*.o
