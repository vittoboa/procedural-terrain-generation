CC=gcc
CFLAGS = -g -Werror -Wall -Wextra -Wfloat-equal -lGL -lglut -lGLEW -lm -lcglm -O
OBJECTS = main.o shader.o terrain.o perlin.o
all: start

start: $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o start

%.o: %.c
	$(CC) -c $<

clean:
	rm start *.o
