all:
	gcc -O2 -Wall -Werror -Wextra sshell.c -o sshell

clean:
	rm -f sshell