# Nombres de los ejecutables
TARGETS = ex5 ex6 ex7 blur edge

# Compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lm

# Archivos fuente
SRC_EX5 = ex5.c bmp.c
SRC_EX6 = ex6.c bmp.c
SRC_EX7 = ex7.c bmp.c
SRC_BLUR = blur.c bmp.c
SRC_EDGE = edge.c bmp.c

# Directorio de ejecutables y objetos
BIN_DIR = executes

# Archivos objeto
OBJ_EX5 = $(addprefix $(BIN_DIR)/, $(SRC_EX5:.c=.o))
OBJ_EX6 = $(addprefix $(BIN_DIR)/, $(SRC_EX6:.c=.o))
OBJ_EX7 = $(addprefix $(BIN_DIR)/, $(SRC_EX7:.c=.o))
OBJ_BLUR = $(addprefix $(BIN_DIR)/, $(SRC_BLUR:.c=.o))
OBJ_EDGE = $(addprefix $(BIN_DIR)/, $(SRC_EDGE:.c=.o))

# Regla principal
all: $(BIN_DIR) $(TARGETS)

# Crear directorio de ejecutables y objetos
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Reglas para cada ejecutable
ex5: $(OBJ_EX5)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

ex6: $(OBJ_EX6)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

ex7: $(OBJ_EX7)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

blur: $(OBJ_BLUR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

edge: $(OBJ_EDGE)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

# Regla general para compilar archivos fuente a objetos
$(BIN_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*

# Test de la compilación con un caso de prueba
test: blur edge ex5
	./$(BIN_DIR)/ex5

test1: blur edge ex6
	./$(BIN_DIR)/ex6 testcases/car.bmp outputs/output.bmp 10

test2: blur edge ex7
	./$(BIN_DIR)/ex7


