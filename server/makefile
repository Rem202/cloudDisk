SRCS:=$(wildcard src/*.c)
OBJS:=$(patsubst src/%.c, src/%.o,$(SRCS))
TARGET:= main
LINK:= -g -pthread -I "include" -w -lmysqlclient -lcrypt


all:$(TARGET)

$(TARGET):$(OBJS)
	gcc $^ -o $@ $(LINK)
%.o:%.c
	gcc -c $^ -o $@ $(LINK)

.PHONY:clean rebuild
clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
rebuild:clean all
