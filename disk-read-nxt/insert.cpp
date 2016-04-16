#include <iostream>
#include <mysqlpp.h>

using namespace mysqlpp;

Connection c;

int
init_tables()
{
	Query q = c.query("CREATE TABLE IF NOT EXISTS clients VALUES(id NOT NULL INT PRIMARY_KEY AUTOINCREMENT, ip BIGINT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS jobs VALUES(id NOT NULL INT PRIMARY_KEY AUTOINCREMENT, cid INT, year SHORTINT, month SHORTINT, day SHORTINT, hour SHORTINT, minute SHORTINT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS jtof VALUES(jid INT, fid INT)");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS files VALUES(id NOT NULL INT PRIMARY_KEY AUTOINCREMENT, name VARCHAR(255), size INT, cid INT, gid INT, uid INT, perm INT, ctime INT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS ftof VALUES(fid1 INT, fid2 INT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS logs VALUES(id NOT NULL INT PRIMARY_KEY AUTOINCREMENT, cid INT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS ftob VALUES(fid INT, bid INT);");
	q.exec();

	q = c.query("CREATE TABLE IF NOT EXISTS blocks VALUES(id NOT NULL PRIMARY_KEY AUTOINCREMENT, data BINARY(4096));");
	q.exec();

	return 0;
}

int
insert_test_data()
{
	Query q = c.query();
	q << "INSERT INTO clients VALUES(NULL," << inet_addr("127.0.0.1") << ");";
	int cid = q.exec().insert_id();

	q = c.query();
	q << "INSERT INTO files VALUES(NULL, \"/home/pi/crowdify/crowdify/disk-read-nxt/test\", 0," << cid << ", 0, 0, 0, 0);";
	int fid = q.exec().insert_id();	

	q = c.query();
	q << "INSERT INTO jobs VALUES(NULL, " << cid << ", 0, 0, 0, 0, 0);";
	int jid = q.exec().insert_id();	

	q = c.query();
	q << "INSERT INTO jtof VALUES(" << jid << "," << fid << ");";
	q.exec();	

	return 0;
}

int
exec_current_job()
{
	
	Query q = c.store("SELECT * FROM jobs;");
	StoreQueryResult store = q.store();

	if (store) {
		for (size_t i = 0; i < res.num_rows(); i++) {
			cout << res[i]["id"] << "," << res[i]["cid"] << "," << res[i]["year"] << "," << res[i]["year"] << "," << res[i]["month"] << "," << res[i]["day"] << "," << res[i]["hour"] << "," << res[i]["day"] << "," << res[i]["minute"] << endl;
		}
	}
	return 0;
}

int
main()
{
	return 0;
}
