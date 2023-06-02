CC = gcc
CFLAGS = -pthread
TARGET = findeq
OUTPUT = output.txt

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

$(TARGET): findeq.o
	$(CC) $(CFLAGS) $^ -o $@

findeq.o: findeq.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
	rm $(OUTPUT)
