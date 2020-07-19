all: predictors

predictors: predictors.o
	g++ -g -Wall -Werror predictors.o -o predictors

predictors.o: predictors.cpp
	g++ -c -g -Wall -Werror predictors.cpp -o predictors.o

.PHONY: clean all

clean:
	rm -rf predictors *.o
