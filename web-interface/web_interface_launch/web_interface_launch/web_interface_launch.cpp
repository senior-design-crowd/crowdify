
#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(void) {
	bool system_ready;
	errno = 0;

	if (FILE *file = fopen("status.txt", "r")) {
		char line[20];
		fgets(line, 20, file);
		if (strcmp(line, "ready\n") == 0) system_ready = true;
		else system_ready = false;
		fclose(file);
	}
	else {
		printf("%d\n", errno);
		system_ready = false;
	}

	if (system_ready == false) {
		printf("Still waiting for server discovery...\n");
		system("pause");
	}
	else {
		system("web_interface.url");
	}
}