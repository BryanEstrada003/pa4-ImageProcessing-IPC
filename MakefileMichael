# Variables
GCC = gcc
CFLAGS = -Wall -Wshadow -pthread
OBJS = ex5.o bmp.o filter.o

# Objetivo principal
ex5: $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $@

# Compilación de los archivos .c a .o
.c.o:
	$(GCC) $(CFLAGS) -c $*.c 

# Limpieza de los archivos objetos y el ejecutable
clean:
	rm -f *.o ex5

# Test de la compilación con un caso de prueba
test: ex5
	./ex5 testcases/test.bmp outputs/test_out.bmp
	diff outputs/test_out.bmp testcases/test_sol.bmp

# Test de memoria con Valgrind
testmem: ex5
	valgrind --tool=memcheck --leak-check=summary ./ex5 testcases/test.bmp outputs/test_out.bmp
