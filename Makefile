All: hello

hello : hello.c
	gcc -o hello hello.c

clean:
	rm -f *~ hello
