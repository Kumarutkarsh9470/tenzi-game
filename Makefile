# Simple Makefile for the ncurses shooting game
CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wconversion -Wshadow -Werror -O2
LDFLAGS := -lncurses
TARGET := plane_defense
SRC := src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean