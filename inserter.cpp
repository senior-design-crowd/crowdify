#include <cstdlib>
#include <iostream>
#include <mongo/client/dbclient.h>
#include <mongo/bson/bson.h>
#include <ftw.h>

using namespace mongo;
using namespace std;

mongo::DBClientConnection c;

int
insert_file_name(const char *fpath, const struct stat *sb, int typeflag)
{
	string *pathed = new string;
	*pathed = fpath;

	if (typeflag == FTW_F) {
		BSONObj path = BSON( "path" << *pathed );

		c.insert("tutorials.pages", path);
	}

	return 0;
}

int
main(void)
{
	mongo::client::initialize();

	BSONObj page = BSON( "path" << "/home/ormris/src/crowdify/inserter.cpp" << "hash" << 15);


	
    try {
		c.connect("localhost");
        std::cout << "connected ok" << std::endl;

		c.insert("tutorial.pages", page);

		ftw(".", insert_file_name, 16);

		auto_ptr<DBClientCursor> cursor = c.query("tutorial.pages", BSONObj());

		while (cursor->more()) {
			cout << cursor->next().toString() << endl;
		}


    } catch( const mongo::DBException &e ) {
        std::cout << "caught " << e.what() << std::endl;
    }
	return EXIT_SUCCESS;
}
