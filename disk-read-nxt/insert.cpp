#include <iostream>
#include <mysql++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace mysqlpp;
using namespace std;

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
	q.exec();
	int cid = q.insert_id();

	q = c.query();
	q << "INSERT INTO files VALUES(NULL, \"/home/pi/crowdify/crowdify/disk-read-nxt/test\", 0," << cid << ", 0, 0, 0, 0);";
	q.exec();
	int fid = q.insert_id();	

	q = c.query();
	q << "INSERT INTO jobs VALUES(NULL, " << cid << ", 0, 0, 0, 0, 0);";
	q.exec();
	int jid = q.insert_id();	

	q = c.query();
	q << "INSERT INTO jtof VALUES(" << jid << "," << fid << ");";
	q.exec();	

	return 0;
}

int
data_compare_ctime(Row *f1, Row *f2)
{
	return 0;
}

int
data_cd(int fid)
{
	return 0;
}

int
data_ls()
{
	return 0;
}
int
data_exists()
{
	return 0;
}

int
init_connection()
{
	c.connect("crowdify", "127.0.0.1", "root", "root");

	return 0;
}

int
exec_current_job()
{
	
	Query q = c.query("SELECT * FROM jobs;");
	StoreQueryResult store = q.store();

	if (store) {
		for (size_t i = 0; i < store.num_rows(); i++) {
			cout << store[i]["id"] << "," << store[i]["cid"] << "," << store[i]["year"] << "," << store[i]["year"] << "," << store[i]["month"] << "," << store[i]["day"] << "," << store[i]["hour"] << "," << store[i]["day"] << "," << store[i]["minute"] << endl;
		}
	}
	return 0;
}

int
main()
{
	init_connection();
	init_tables();
	insert_test_data();
	exec_current_job();
	return 0;
}
