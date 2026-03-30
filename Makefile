CXX = g++
CXXFLAGS = -Wall -Wextra

TARGET = team11_VMCacheSim
SRC = team11_VMCacheSim.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
