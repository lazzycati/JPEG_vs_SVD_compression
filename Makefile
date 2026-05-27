CC = gcc 
CFLAGS = -Wall -Wextra -O2 -I. -Iauxiliary -Icompare -Ijpeg -Isample -Isvd
LDFLAGS = -lm 
SOURCES = $(wildcard auxiliary/*.c compare/*.c jpeg/*.c sample/*.c svd/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = main
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f compr_results.csv
rebuild: clean all
.PHONY: all clean rebuild