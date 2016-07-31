OBJ = polysporth.o dvector.o ps.o scheme.o f.o 

CFLAGS=-DSTANDALONE=0 -fPIC -g 

# Uncomment to put polysporth in debug mode
#CFLAGS+=-DDEBUG_POLYSPORTH

default: polysporth.so

%.o: %.c 
	$(CC) $(CFLAGS) -c $^ 

polysporth.so: $(OBJ)
	gcc $(CFLAGS) $(OBJ) -o $@ -ldl -lsporth -lsoundpipe -shared

clean:
	rm -rf *.o *.so
