OBJ = polysporth.o dvector.o ps.o scheme.o f.o 

CFLAGS=-DSTANDALONE=0  -fPIC

# Uncomment to put polysporth in debug mode
#CFLAGS+=-DDEBUG_POLYSPORTH

default: polysporth.so

%.o: %.c 
	$(CC) $(CFLAGS) -c $^ 

polysporth.so: $(OBJ)
	gcc -shared $(CFLAGS) $(OBJ) -o $@ -lsporth -lsoundpipe -ldl -lm 

clean:
	rm -rf *.o *.so
