lispy: lispy.c
	gcc -Wall -o lispy lispy.c mpc.c -lreadline -lm
debug: lispy.c
	gcc -Wall -g -o lispy lispy.c mpc.c -lreadline -lm
