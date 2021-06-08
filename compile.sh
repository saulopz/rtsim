#!/bin/sh
clang++ -Wall -fexceptions -std=c++14 \
        -g src/*.cpp -o rtsim \
        -lalleg -lm -lXxf86vm -lXcursor -lXpm -lXext -lX11 -lpthread -ldl

