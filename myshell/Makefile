CC = gcc

CFLAGS = -ggdb -MMD -MP
LDFLAGS =

TARGET = mysh
MYSH_OBJS = main.o proc.o

SRCS = main.c proc.c
OBJS = $(MYSH_OBJS)
DEPS = $(SRCS:%.c=%.d)

RM = rm -rf

.PHONY: all clean

all: $(TARGET)

mysh: $(MYSH_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
