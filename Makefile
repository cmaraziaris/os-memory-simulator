PROGRAM = mem_sim

CC = gcc
CFLAGS = -Wall -Wextra -I. 

OBJS = simulator.o memory.o page_repl.o queue.o

$(PROGRAM): clean $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJS)

# default arguments
run: $(PROGRAM)
	./$(PROGRAM) LRU 20 10