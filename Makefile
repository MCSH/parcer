

all: build test

build: test.cpp parcer.h
	g++ test.cpp -o test

test: build
	./test
