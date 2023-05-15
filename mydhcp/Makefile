CC = gcc

CFLAGS = -ggdb -MMD -MP
LDFLAGS =

TARGET = mydhcpd mydhcpc
MYDHCPD_OBJS = mydhcpd.o
MYDHCPC_OBJS = mydhcpc.o

SRCS = mydhcpd.c mydhcpc.c 
OBJS = $(MYDHCPD_OBJS) $(MYDHCPC_OBJS)
DEPS = $(SRCS:%.c=%.d)

RM = rm -rf
  
.PHONY: all clean

all: $(TARGET)

mydhcpd: $(MYDHCPD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

mydhcpc: $(MYDHCPC_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)