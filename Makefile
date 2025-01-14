# Nombres de los ejecutables
TARGETS = publicador_combinador desenfocador realzador

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Archivos fuente
SRC_PUBLICADOR = publicador_combinador.c
SRC_DESENFOCADOR = desenfocador.c
SRC_REALZADOR = realzador.c

# Objetos generados
OBJ_PUBLICADOR = publicador_combinador.o
OBJ_DESENFOCADOR = desenfocador.o
OBJ_REALZADOR = realzador.o

# Regla principal
all: $(TARGETS)

# Compilar el publicador_combinador
publicador_combinador: $(OBJ_PUBLICADOR)
	$(CC) $(CFLAGS) -o $@ $^

# Compilar el desenfocador
desenfocador: $(OBJ_DESENFOCADOR)
	$(CC) $(CFLAGS) -o $@ $^

# Compilar el realzador
realzador: $(OBJ_REALZADOR)
	$(CC) $(CFLAGS) -o $@ $^

# Generar objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(TARGETS) *.o

# Limpiar todo
mrproper: clean
	rm -f salida.bmp
