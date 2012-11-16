#include <stdio.h>
#include <termios.h>

int main(int argc, char* argv[]) {

	struct termios old, new;

	tcgetattr(0, &old);
	new = old;

	new.c_lflag &=(~ICANON & ~ECHO);

	tcsetattr(0, TCSANOW, &new);

	while(1) {
		char c = getchar();
		if(c != 10) {
			fprintf(stderr, "%d\n", (int)c);
		} else {
			fprintf(stderr, "\n");
		}
	}

	tcsetattr(0, TCSANOW, &old);

	return 0;
}
