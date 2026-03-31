src_dir=./src
obj_dir=./obj
include_dir=./include

# 一级子目录下的全部 .c（app / config / event / http / util / stats）
src=$(wildcard $(src_dir)/*/*.c)
obj=$(patsubst $(src_dir)/%.c,$(obj_dir)/%.o,$(src))

ALL : server.out

server.out : ${obj}
	gcc $^ -o $@ -levent -g 

$(obj_dir)/%.o : $(src_dir)/%.c
	@mkdir -p $(dir $@)
	gcc -c $< -o $@ -I ${include_dir} -g 

.PHONY: clean

clean:
	rm -rf $(obj_dir)
	rm -f server.out
