#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"



char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**) malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


int main(int argc, char *argv[]){
	char* add_dirs_string = argv[1];
	char* remove_dirs_string = argv[2];
	char* add_excludes_string = argv[3];
	char* remove_excludes_string = argv[4]
	char* date_and_time = argv[5];
	char* userName = argv[6];
	char* addr = argv[7];
	char** add_dirs;
	char** remove_dirs;
	char** add_excludes;
	char** remove_excludes;
	
	add_dirs = str_split(add_dirs_string, ',');
	remove_dirs = str_split(remove_dirs_string, ',');
	add_excludes = str_split(add_excludes_string, ',');
	remove_excludes = str_split(remove_excludes_string, ',');

	printf("In mod_dirs\n");

    if (add_dirs)
    {
		ClientConfig* cc = new ClientConfig(addr, userName);
        int i;
        for (i = 0; *(dirs + i); i++)
        {
			cc->insertDirectory(*(add_dirs + i));
		printf("Added %s\n", *(add_dirs + i));
            free(*(add_dirs + i));
        }
        free(add_dirs);
    }
	if (remove_dirs)
    {
		ClientConfig* cc = new ClientConfig(addr, userName);
        int i;
        for (i = 0; *(remove_dirs + i); i++)
        {
			cc->removeDirectory(*(remove_dirs + i));
		printf("Added %s\n", *(remove_dirs + i));
            free(*(remove_dirs + i));
        }
        free(remove_dirs);
    }
	if (add_excludes)
    {
		ClientConfig* cc = new ClientConfig(addr, userName);
        int i;
        for (i = 0; *(add_excludes + i); i++)
        {
			cc->addExclude(*(add_excludes + i));
		printf("Added %s\n", *(add_excludes + i));
            free(*(add_excludes + i));
        }
        free(add_excludes);
    }

	if (remove_excludes)
    {
		ClientConfig* cc = new ClientConfig(addr, userName);
        int i;
        for (i = 0; *(remove_excludes + i); i++)
        {
			cc->removeExclude(*(remove_excludes + i));
		printf("Added %s\n", *(remove_excludes + i));
            free(*(remove_excludes + i));
        }
        free(remove_excludes);
    }
	
	if(strcmp(date_and_time, "UNDEFINED") != 0){
		ClientConfig* cc = new ClientConfig(addr, userName);
		cc->setStartDateAndTime(date_and_time);
		printf("Added %s\n", date_and_time);
	}

	return 0;
}