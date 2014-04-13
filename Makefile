lispy: lispy.c mpc.c
	gcc -Wall -o lispy lispy.c mpc.c -lreadline -lm
