#!/bin/sh
gcc -lm -lstdc++ -std=c++0x -lmpi ./debugVisualization.cpp ./nodeMessages.cpp ./DHTArea.cpp ./main.cpp ./Node.cpp -o dhtnode