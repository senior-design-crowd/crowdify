#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"

int main(int argc, char *argv[]){
	char* date = argv[1];
	char* dirs = argv[2];
	char* userName = argv[3];
	char* addr = argv[4];
	
	printf("in restore\n");
	ClientConfig* cc = new ClientConfig(addr, userName);
	cc->restore(date, dirs);
	return 0;
}