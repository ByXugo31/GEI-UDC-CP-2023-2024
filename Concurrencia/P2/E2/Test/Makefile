# Definición de variables
CC = gcc
CFLAGS = -Wall -I -g

# Lista de archivos fuente y objetos
SRCS = test.c queue.c
OBJS = $(SRCS:.c=.o)

# Nombre del programa de salida
TARGET = TEST

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependencias de los archivos de encabezado
test.o: test.c queue.h
queue.o: queue.c queue.h

# Lmpiar archivos temporales
clean:
	rm -f $(OBJS) $(TARGET)