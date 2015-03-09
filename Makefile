CXX = c++
CXXFLAGS = -std=c++1y -pthread

all: clean client

client:
	$(CXX) $(CXXFLAGS) client.cpp -o client

clean:
	rm -f client server a.out *.o