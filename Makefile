TARGET=MyStrategy
CXX=g++
CXXFLAGS=-std=c++11 -static -fno-optimize-sibling-calls -fno-strict-aliasing -DONLINE_JUDGE -D_LINUX -DSLAVA_DEBUG -lm -s -O2 -Wall

OBJECTS=Runner.o Strategy.o csimplesocket/ActiveSocket.o csimplesocket/HTTPActiveSocket.o csimplesocket/PassiveSocket.o csimplesocket/SimpleSocket.o model/Bonus.o model/PlayerContext.o model/Player.o model/Unit.o model/Game.o model/World.o model/Move.o model/Trooper.o RemoteProcessClient.o

.PHONY: all run clean $(TARGET)

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(TARGET).cpp -o $@

run:
	@./local-runner/run.pl

clean:
	$(RM) $(OBJECTS) $(TARGET)
