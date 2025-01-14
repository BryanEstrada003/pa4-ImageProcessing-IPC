# Nombres de los ejecutables
TARGETS = publicador_combinador desenfocador realzador ex5

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

# Archivos fuente
SRC_PUBLICADOR = publicador_combinador.c
SRC_DESENFOCADOR = desenfocador.c
SRC_REALZADOR = realzador.c
SRC_EX5 = ex5.c bmp.c filter.c

# Archivos objeto
OBJ_PUBLICADOR = $(SRC_PUBLICADOR:.c=.o)
OBJ_DESENFOCADOR = $(SRC_DESENFOCADOR:.c=.o)
OBJ_REALZADOR = $(SRC_REALZADOR:.c=.o)
OBJ_EX5 = $(SRC_EX5:.c=.o)

# Regla principal
all: $(TARGETS)

# Reglas para cada ejecutable
publicador_combinador: $(OBJ_PUBLICADOR)
	$(CC) $(CFLAGS) -o $@ $^

desenfocador: $(OBJ_DESENFOCADOR)
	$(CC) $(CFLAGS) -o $@ $^

realzador: $(OBJ_REALZADOR)
	$(CC) $(CFLAGS) -o $@ $^

ex5: $(OBJ_EX5)
	$(CC) $(CFLAGS) -o $@ $^

# Regla general para compilar archivos fuente a objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f *.o $(TARGETS)

# Test de la compilación con un caso de prueba
test: ex5
	./ex5 testcases/car.bmp outputs/output.bmp
#	diff outputs/test_out.bmp testcases/test_sol.bmp

# Test de memoria con Valgrind
testmem: ex5
	valgrind --tool=memcheck --leak-check=summary ./ex5 testcases/test.bmp outputs/test_out.bmp
