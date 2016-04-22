#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../disk-read-ng/configsql.h"


void main(int argc, char *argv[]){
	char* add_dirs_string = argv[1];
	char* remove_dirs_string = argv[2];
	char* userName = argv[3];
	char* addr = argv[4];
	char** add_dirs;
	char** remove_dirs;
	
	add_dirs = str_split(add_dirs_string, ',');
	remove_dirs = str_split(remove_dirs_string, ',');

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
}

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

    result = malloc(sizeof(char*) * count);

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