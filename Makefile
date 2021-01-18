G = g++-4.9
CFLAGS = -I/usr/local/lib/Oracle/instantclient_11_2/sdk/include
LFLAGS = -L/usr/local/lib/Oracle/instantclient_11_2 -locci -lociei

all: vet

vet.o: vet.cpp
	$(G) -c $(CFLAGS) vet.cpp

vet: vet.o
	$(G) $(LFLAGS) -o vet vet.o 
