#include "configsql.h"

int
main()
{
	ClientConfig c("127.0.0.1", "root");
	c.insertDirectory("/root/tmp/");

	return 0;
}
