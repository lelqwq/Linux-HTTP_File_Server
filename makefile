src_dir=./src
obj_dir=./obj
header_dir=./header

src=$(wildcard $(src_dir)/*.c)
obj=$(patsubst $(src_dir)/%.c, $(obj_dir)/%.o, $(src))

ALL : server.out

server.out : ${obj}
	gcc $^ -o $@ -levent -g 

$(obj_dir)/%.o : $(src_dir)/%.c
	gcc -c $< -o $@ -I ${header_dir} -g 

.PHONY: clean

clean:
	rm -f $(obj_dir)/*
	rm -f server.out