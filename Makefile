all: out/reader out/writer

clean: rm -f out/*

out/reader: src/reader.cpp
	g++ -g -Wall -Wextra -o out/reader src/reader.cpp

out/writer: src/writer.cpp
	g++ -g -Wall -Wextra -o out/writer src/writer.cpp