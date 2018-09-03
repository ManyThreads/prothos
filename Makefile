CC := g++
CFLAGS := -Wall -std=c++14 -g -pthread
LDFLAGS := -g -lpthread -lbenchmark

SRCEXT := cc
SRCDIR := src
INCDIR := inc
BUILDDIR := bin
TARGET := main

SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%.o,$(BUILDDIR)/%.o,$(SOURCES:.$(SRCEXT)=.o))


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CC) $< $(CFLAGS) -c -I$(INCDIR) -o $@

clean:
	-rm $(TARGET) $(OBJECTS)

run: all
	./$(TARGET)
