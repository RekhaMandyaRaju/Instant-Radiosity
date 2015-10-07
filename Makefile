MK_DIR = mkdir -p
CC = g++
DEBUG = -O2
CFLAGS = -Wall $(DEBUG)
LFLAGS = 
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

all: directories $(patsubst %.cpp, %.o, $(wildcard, *.cpp)) CS580HW

directories: 
	${MK_DIR} obj

obj/%.o: %.cpp Makefile
	$(CC) $(CFLAGS) -c -o $@ $<  -lm

CS580HW: $(OBJ_FILES)
	$(CC) $(LFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -rf obj/*.o
	rm -f CS580HW
	rm -f output.ppm

CFLAGS += -MMD
-include $(OBJ_FILES:.o=.d)
