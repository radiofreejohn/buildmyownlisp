lispy: lispy.c mpc.c list.c
	gcc -Wall -o lispy lispy.c mpc.c list.c -lreadline -lm
debug: lispy.c mpc.c list.c
	gcc -Wall -g -o lispy lispy.c mpc.c list.c -lreadline -lm
test: tests.c lispy.c mpc.c list.c ptest.c
	gcc -Wall -Wno-incompatible-function-pointer-types -DLISPY_TEST -o test_runner tests.c lispy.c mpc.c list.c ptest.c -lreadline -lm
	./test_runner
clean:
	rm -f lispy test_runner
