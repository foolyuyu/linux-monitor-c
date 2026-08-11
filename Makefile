CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = monitor
SRV = monitor.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) monitor.c -o monitor

clean:
	rm -f $(TARGET)


