all: a.out

clean:
	rm -f a.out

a.out: main.cpp
	g++ -g -Wall -Wextra -pedantic -Weffc++ -O3 main.cpp

run: a.out
	./a.out
