object = main.o thread.o init.o core.o timer.o sign.o 

server:$(object)
	   gcc -pthread $(object) -o server -lmysqlclient -DNDEBUG 

$(object):%.o:%.c
	   gcc -c $< -o $@

.PHONY:clean
clean:
	rm server $(object)
