#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"

int main(int argc, char *argv[]){
	char* userName = argv[1];
	char* addr = argv[2];
	
	ClientConfig* cc = new ClientConfig(addr, userName);
	cc->backupNow();
	return 0;
}