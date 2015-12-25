CXX = c++
CXXFLAGS = -std=c++1y -pthread

all: clean client center

client:
	$(CXX) $(CXXFLAGS) src/client.cpp -o client

center:
	$(CXX) $(CXXFLAGS) src/server.cpp -o center

clean:
	rm -f client center a.out *.o
