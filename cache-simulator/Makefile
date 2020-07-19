all: cache-sim

cache-sim: cache-sim.o
	g++ -g -Wall -Werror cache-sim.o -o cache-sim

cache-sim.o: cache-sim.cpp
	g++ -c -g -Wall -Werror cache-sim.cpp -o cache-sim.o

.PHONY: clean all

clean:
	rm -rf cache-sim *.o
