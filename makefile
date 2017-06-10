#Compiler flags:
# -g    adds debugging information to the executable file
# -Wall turns on most, but not all, compiler warnings
CFLAGS = -Wall -g

#Extra flags to give to compilers when they are supposed to invoke the linker, ‘ld’, such as -L.
#Libraries (-lfoo) should be added to the LDLIBS variable instead.
#LDFLAGS = 

#Library flags or names given to compilers when they are supposed to invoke the linker, ‘ld’.
#Non-library linker flags, such as -L, should go in the LDFLAGS variable.
LDLIBS = -lwiringPi -lmpdclient

#The target executable file name
TARGET = rad10d

# The compiler; typically gcc for c and g++ for c++
CC = gcc

all: rotaryencoder/rotaryencoder.o
	$(CC) $(CFLAGS) $(LDLIBS) $(TARGET).c rotaryencoder/rotaryencoder.o -o $(TARGET)

rotaryencoder/rotaryencoder.o: rotaryencoder/rotaryencoder.c
	$(CC) $(CFLAGS) -c rotaryencoder/rotaryencoder.c -o rotaryencoder/rotaryencoder.o

clean:
	rm -f $(TARGET) rotaryencoder/rotaryencoder.o *.o

