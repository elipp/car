CC=g++ -g

#g++ main.cpp -Wall net/server.cpp net/client.cpp -lGL -lGLU -lSDL -lm -DOGLFT_NO_SOLID -DOGLFT_NO_QT -I/usr/include/freetype2 OGLFT.o -lfreetype -o joo

CFLAGS=-c -Wall
LIBS=-lGL -lGLU -lSDL -lm -lfreetype -lrt
SOURCES=src/main.cpp src/net/server.cpp src/net/client.cpp
INCLUDE=-I/usr/include/freetype2
OBJS=server.o client.o
OBJDIR=objs
DEFINES=-DOGLFT_NO_SOLID -DOGLFT_NO_QT
objects = $(addprefix $(OBJDIR)/, $(OBJS))
EXECUTABLE=joo

all: joo 

objdir:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

joo: objdir $(objects)
	$(CC) $(LIBS) $(DEFINES) $(objects) OGLFT.o src/main.cpp $(INCLUDE) -o $(EXECUTABLE)

$(OBJDIR)/client.o: src/net/client.cpp
	$(CC) $(CFLAGS) $(LIBS) $< -o $@

$(OBJDIR)/server.o: src/net/server.cpp
	$(CC) $(CFLAGS) $(LIBS) $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(OBJDIR)/*.o
