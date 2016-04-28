#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"


int main(int argc, char *argv[]){
	char* userName = argv[1];
	char* addr = argv[2];
	char* backupDirsString;
	char* dates;

	printf("In getBackupDirs\n");
	
	ClientConfig* cc = new ClientConfig(addr, userName);
    backupDirsString = cc->getBackupDirs();
	dates = cc->getBackupDates();
	
	FILE* file = fopen("files.txt", "w+");
	fprintf(file, backupDirsString);
	fclose(file);
	file = fopen("dates.txt", "w+");
	fprintf(file, dates);
	fclose(file);
	return 0;
}