CC = gcc
CFLAGS = -pthread
TARGET = findeq

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

$(TARGET): findeq.o
	$(CC) $(CFLAGS) $^ -o $@

findeq.o: findeq.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
