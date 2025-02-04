# Nombres de los ejecutables
TARGETS = ex7

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lm

# Archivos fuente
SRC_EX7 = ex7.c bmp.c

# Directorio de ejecutables y objetos
BIN_DIR = executes

# Archivos objeto
OBJ_EX7 = $(addprefix $(BIN_DIR)/, $(SRC_EX7:.c=.o))

# Regla principal
all: $(BIN_DIR) $(TARGETS)

# Crear directorio de ejecutables y objetos
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Reglas para cada ejecutable
ex7: $(OBJ_EX7)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

# Regla general para compilar archivos fuente a objetos
$(BIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*

# Test de la compilación con un caso de prueba
test: ex7
	./$(BIN_DIR)/ex7


