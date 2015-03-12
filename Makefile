CXX = c++
CXXFLAGS = -std=c++1y -pthread

all: clean client center

client:
	$(CXX) $(CXXFLAGS) client.cpp -o client

center:
	$(CXX) $(CXXFLAGS) server.cpp -o center

clean:
	rm -f client center a.out *.o