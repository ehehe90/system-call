CC = gcc

CFLAGS = -ggdb -MMD -MP
LDFLAGS =

TARGET = myftpd myftpc
MYFTPD_OBJS = myftpd.o myftpd_proc.o tcp.o print_files.o
MYFTPC_OBJS = myftpc.o myftpc_proc.o tcp.o print_files.o

SRCS = myftpd.c myftpd_proc.c myftpc.c myftpc_proc.c tcp.c print_files.c
OBJS = $(MYFTPD_OBJS) $(MYFTPC_OBJS)
DEPS = $(SRCS:%.c=%.d)

RM = rm -rf
  
.PHONY: all clean

all: $(TARGET)

myftpd: $(MYFTPD_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

myftpc: $(MYFTPC_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)