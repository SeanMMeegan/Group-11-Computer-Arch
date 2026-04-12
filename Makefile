# === Compiler ===
CXX = g++

# === Flags ===
CXXFLAGS = -std=c++11 -Wall -O2

# === Target executable ===
TARGET = team11_VMCacheSim_M2

# === Source file ===
SRC = team11_VMCacheSim_M2.cpp

# === Build rule ===
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# === Clean rule ===
clean:
	rm -f $(TARGET)
