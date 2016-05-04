#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"


int main(int argc, char *argv[]){
	char* userName = argv[1];
	char* addr = argv[2];
	char* backupDirsString;
	char* all_dirs;
	char* dates;

	printf("In getBackupDirs\n");
	
	ClientConfig* cc = new ClientConfig(addr, userName);
    backupDirsString = cc->getBackupDirs();
	all_dirs = cc->getAllDirs();
	dates = cc->getBackupDates();
	
	FILE* file = fopen("dirs.txt", "w+");
	fprintf(file, backupDirsString);
	fclose(file);
	file = fopen("all_dirs.txt", "w+");
	fprintf(file, all_dirs);
	fclose(file);
	file = fopen("dates.txt", "w+");
	fprintf(file, dates);
	fclose(file);
	return 0;
}