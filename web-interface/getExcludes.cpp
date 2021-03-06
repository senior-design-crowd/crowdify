#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"


int main(int argc, char *argv[]){
	char* userName = argv[1];
	char* addr = argv[2];
	char* excludesString;

	printf("In getExcludes\n");
	
	ClientConfig* cc = new ClientConfig(addr, userName);
    excludesString = cc->getExcludes();
	
	FILE* file = fopen("excludes.txt", "w+");
	fprintf(file, excludesString);
	fclose(file);
	return 0;
}