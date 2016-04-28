#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"


int main(int argc, char *argv[]){
	char* date = argv[1];
	char* userName = argv[2];
	char* addr = argv[3];
	char* dirsString;

	printf("In getDirsForDate\n");
	
	ClientConfig* cc = new ClientConfig(addr, userName);
    dirsString = cc->getDirsForDate(date);
	
	char file_name[1000];
	strcpy(file_name, date);
	strcat(file_name, ".txt");)
	
	FILE* file = fopen(file_name, "w+");
	fprintf(file, dirsString);
	fclose(file);
	return 0;
}