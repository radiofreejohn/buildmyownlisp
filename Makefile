lispy: lispy.c mpc.c list.c
	gcc -Wall -Wno-incompatible-function-pointer-types -o lispy lispy.c mpc.c list.c -lreadline -lm -lpthread
debug: lispy.c mpc.c list.c
	gcc -Wall -g -o lispy lispy.c mpc.c list.c -lreadline -lm -lpthread
test: tests.c lispy.c mpc.c list.c ptest.c
	gcc -Wall -Wno-incompatible-function-pointer-types -DLISPY_TEST -o test_runner tests.c lispy.c mpc.c list.c ptest.c -lreadline -lm -lpthread
	./test_runner
clean:
	rm -f lispy test_runner
