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

# Directorio de ejecutables y objetos
BIN_DIR = executes

# Archivos objeto
OBJ_PUBLICADOR = $(addprefix $(BIN_DIR)/, $(SRC_PUBLICADOR:.c=.o))
OBJ_DESENFOCADOR = $(addprefix $(BIN_DIR)/, $(SRC_DESENFOCADOR:.c=.o))
OBJ_REALZADOR = $(addprefix $(BIN_DIR)/, $(SRC_REALZADOR:.c=.o))
OBJ_EX5 = $(addprefix $(BIN_DIR)/, $(SRC_EX5:.c=.o))
OBJ_EX6 = $(addprefix $(BIN_DIR)/, $(SRC_EX6:.c=.o))

# Regla principal
all: $(BIN_DIR) $(TARGETS)

# Crear directorio de ejecutables y objetos
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Reglas para cada ejecutable
publicador_combinador: $(OBJ_PUBLICADOR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^

desenfocador: $(OBJ_DESENFOCADOR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^

realzador: $(OBJ_REALZADOR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^

ex5: $(OBJ_EX5)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

ex6: $(OBJ_EX6)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

# Regla general para compilar archivos fuente a objetos
$(BIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*

# Test de la compilación con un caso de prueba
test1: ex5
	./$(BIN_DIR)/ex5 testcases/car.bmp outputs/output.bmp

test2: ex6
	./$(BIN_DIR)/ex6 testcases/car.bmp outputs/output2.bmp

# Test de memoria con Valgrind
testmem: ex5
	valgrind --tool=memcheck --leak-check=summary ./$(BIN_DIR)/ex5 testcases/test.bmp outputs/test_out.bmp
