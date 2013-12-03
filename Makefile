all: a.out

clean:
	rm -f a.out

a.out: main.cpp
	g++ -g main.cpp

run: a.out
	./a.out
