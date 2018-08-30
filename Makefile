.PHONY:httpd clean

src = main.c server.c
head = httpd.h
obj = main.o server.o
bin = httpd
cc = gcc
lib = -lpthread

$(bin) : $(obj)
	$(cc) $^ -o $@ $(lib)
# 代表从0bj中匹配的所有的.o文件都转化为.c文件
$(obj):%.o : %.c $(head)
	$(cc) -c $< -o $@ 

clean:
	rm -rf $(obj) $(bin)
