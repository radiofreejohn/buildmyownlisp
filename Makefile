lispy: lispy.c mpc.c list.c
	gcc -Wall -o lispy lispy.c mpc.c list.c -lreadline -lm
debug: lispy.c mpc.c list.c
	gcc -Wall -g -o lispy lispy.c mpc.c list.c -lreadline -lm
