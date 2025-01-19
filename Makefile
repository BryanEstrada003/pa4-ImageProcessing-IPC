# Nombres de los ejecutables
TARGETS = publicador_combinador desenfocador realzador ex5 ex6

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lm

# Archivos fuente
SRC_PUBLICADOR = publicador_combinador.c
SRC_DESENFOCADOR = desenfocador.c
SRC_REALZADOR = realzador.c
SRC_EX5 = ex5.c bmp.c filter.c edge.c
SRC_EX6 = ex6.c bmp.c filter.c edge.c

# Archivos objeto
OBJ_PUBLICADOR = $(SRC_PUBLICADOR:.c=.o)
OBJ_DESENFOCADOR = $(SRC_DESENFOCADOR:.c=.o)
OBJ_REALZADOR = $(SRC_REALZADOR:.c=.o)
OBJ_EX5 = $(SRC_EX5:.c=.o)
OBJ_EX6 = $(SRC_EX6:.c=.o)

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
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

ex6: $(OBJ_EX6)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regla general para compilar archivos fuente a objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f *.o $(TARGETS)

# Test de la compilación con un caso de prueba
test1: ex5
	./ex5 testcases/car.bmp outputs/output.bmp
#	diff outputs/test_out.bmp testcases/test_sol.bmp

test2: ex6
	./ex6 testcases/car.bmp outputs/output2.bmp

# Test de memoria con Valgrind
testmem: ex5
	valgrind --tool=memcheck --leak-check=summary ./ex5 testcases/test.bmp outputs/test_out.bmp
