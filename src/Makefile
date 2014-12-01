CPPFLAGS=-g -O2   -DNDEBUG  -std=c++11
LDFLAGS=-g -O2  -DNDEBUG
LDLIBS=-Wall

main: main.o dp.o grammar.o features.o sentence.o
	g++ $(LDFLAGS) -o parser main.o dp.o grammar.o features.o sentence.o $(LDLIBS)

tester: dp_test.o dp.o grammar.o
	g++ $(LDFLAGS) -o dp_test dp.o grammar.o  dp_test.o $(LDLIBS)

main.o: main.cpp
	g++ $(CPPFLAGS) -c main.cpp

dp.o: dp.cpp dp.hpp
	g++ $(CPPFLAGS) -c dp.cpp

dp_test.o: dp_test.cpp
	g++ $(CPPFLAGS) -c dp_test.cpp

grammar.o: grammar.cpp grammar.hpp
	g++ $(CPPFLAGS) -c grammar.cpp

features.o: features.cpp features.hpp
	g++ $(CPPFLAGS) -c features.cpp

sentence.o: sentence.cpp sentence.hpp
	g++ $(CPPFLAGS) -c sentence.cpp

clean :
	rm *.o
	rm parser
