
PROGRAM = mem_sim

CC = gcc

CFLAGS = -I. -I./page_repl_algorithms -I./queue -I./memory

OBJS = ./simulator.o ./memory/memory.o ./memory/ipt_management.o \
			 ./page_repl_algorithms/page_repl.o ./queue/queue.o

$(PROGRAM): clean $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJS)

# default arguments
run: $(PROGRAM)
	./$(PROGRAM) LRU 200 10