CC=g++ -g

#g++ main.cpp -Wall net/server.cpp net/client.cpp -lGL -lGLU -lSDL -lm -DOGLFT_NO_SOLID -DOGLFT_NO_QT -I/usr/include/freetype2 OGLFT.o -lfreetype -o joo

CFLAGS=-c -Wall
LIBS=-lGL -lGLU -lSDL -lm -lrt
SOURCES=src/main.cpp src/net/server.cpp src/net/client.cpp
OBJS=server.o client.o
OBJDIR=objs
objects = $(addprefix $(OBJDIR)/, $(OBJS))
EXECUTABLE=joo

all: joo 

objdir:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

joo: objdir $(objects)
	$(CC) $(objects) src/main.cpp -o $(EXECUTABLE) $(LIBS) 

$(OBJDIR)/client.o: src/net/client.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) 

$(OBJDIR)/server.o: src/net/server.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) 

clean:
	rm -rf $(EXECUTABLE) $(OBJDIR)/*.o
