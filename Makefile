CXX = g++
SFML_PREFIX = $(shell /opt/homebrew/bin/brew --prefix sfml 2>/dev/null || brew --prefix sfml 2>/dev/null || echo /usr/local)
CXXFLAGS = -std=c++17 -Wall -I$(SFML_PREFIX)/include
LDFLAGS = -L$(SFML_PREFIX)/lib -lsfml-graphics -lsfml-window -lsfml-system

chess: src/chess.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f chess
