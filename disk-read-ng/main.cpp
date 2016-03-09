#include <iostream>
#include <string>
#include <sstream>
#include <mysql++.h>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mhash.h>
#include <ftw.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace mysqlpp;
using namespace std;

Connection c;
LIBSSH2_SESSION *ssh_session;
LIBSSH2_SFTP  *sftp_session;

int create_test_data() {
	stringstream qs;

	Query q = c.query("CREATE TABLE IF NOT EXISTS clients (cid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, ip BIGINT, username VARCHAR(32), password VARCHAR(16));");

	q.execute();

	q = c.query("CREATE TABLE IF NOT EXISTS directories (did INT AUTO_INCREMENT PRIMARY KEY NOT NULL, path VARCHAR(4096), uid SMALLINT, gid SMALLINT, perm SMALLINT);");
	q.execute();

	q = c.query("CREATE TABLE IF NOT EXISTS clientstodirectories (cid INT, did INT);");
	q.execute();

	q = c.query("SELECT * FROM files");
	qs.clear();
	qs.str(string());
	qs << "INSERT INTO directories VALUES(NULL,\"/root/src/crowdify/disk-read-ng/test/\"," << 0 << "," << 0<< "," << 0 << ");";
	int did = q.execute(qs.str()).insert_id();
	cerr << qs.str() << ":" << did << endl;

	qs.clear();
	qs.str(string());
	qs << "INSERT INTO clients VALUES(NULL," << inet_addr("127.0.0.1") << ",\"root\",\"root\");";
	q.execute(qs.str());
	cerr << qs.str() << endl;

	qs.clear();
	qs.str(string());
	qs << "INSERT INTO clients VALUES(NULL," << inet_addr("127.0.0.1") << ",\"root\",\"root\");";
	int cid = q.execute(qs.str()).insert_id();
	cerr << qs.str() << endl;

	qs.clear();
	qs.str(string());
	qs << "INSERT INTO clientstodirectories VALUES(" << cid << "," << did << ");";
	q.execute(qs.str());
	cerr << qs.str() << endl;
}

int insert_blocks(int fid, const char *fpath) {
	cerr << "file name: " << fpath << endl;
	Query q = c.query("CREATE TABLE IF NOT EXISTS blocks (bid BIGINT, data BLOB(4096));");

	q.execute();

	q = c.query("CREATE TABLE IF NOT EXISTS filestoblocks (fid INT, inc INT, bid BIGINT);");

	q.execute();

	LIBSSH2_SFTP_HANDLE *fd;
	if ((fd = ::libssh2_sftp_open(sftp_session, fpath, 512, LIBSSH2_FXF_READ)) == NULL) {
		cerr << "couldn't open file" << endl;
		return 1;
	}

	char *buf = (char *) calloc(4096, 1);

	int rc = 0;
	int i = 0;

	cerr << "REFERENCE:" << endl;
	cerr << "LIBSSH2_ERROR_SOCKET_SEND = " << LIBSSH2_ERROR_SOCKET_SEND << endl;
	cerr << "LIBSSH2_ERROR_CHANNEL_CLOSED = " << LIBSSH2_ERROR_CHANNEL_CLOSED << endl;
	cerr << "LIBSSH2_ERROR_EAGAIN = " << LIBSSH2_ERROR_EAGAIN << endl;


	while ((rc = ::libssh2_sftp_read(fd, buf, 4096)) > 0) {

		MHASH td = mhash_init(MHASH_CRC32);

		mhash(td, buf, 4096);

		unsigned int hash;
		mhash_deinit(td, &hash);

		std::string d;
		d.assign(buf, sizeof(buf));

		std::stringstream qs;

		const char *ch = strdup(d.c_str());
		string n;
		q.escape_string(&n, ch, strlen(ch) + 1);
		qs << "INSERT INTO blocks VALUES(" << hash << ",\"" << n << "\");";
		cerr << "query: " << qs.str() << endl;

		q.execute(qs.str());

		std::stringstream qss;

		qss << "INSERT INTO filestoblocks VALUES(" << fid << "," << i << "," << hash << ");";
		cerr << "query: " << qss.str() << endl;

		q.execute(qss.str());

		free(buf);
		char *buf = new char[4096];
		i++;
	}

	cerr << "read exited with error code: " << rc << endl;

	::libssh2_sftp_close(fd);
}

int ssh_ftw(int did) {
	stringstream qs;

	qs.clear();
	qs.str(string());
	qs << "SELECT * FROM directories WHERE did=" << did << ";";
	Query q = c.query(qs.str());
	cerr << qs.str() << endl;

	StoreQueryResult r;
	StoreQueryResult::iterator it;
	
	r = q.store();

	char *dirpath;

	cerr << "Opening: " << dirpath << endl;
	for (it = r.begin(); it != r.end(); it++) {
		Row r = *it;
		dirpath = strdup(r[1]);
	}

	sftp_session = libssh2_sftp_init(ssh_session);
	if (sftp_session == NULL) {
		cerr << "couldn't init sftp" << endl;
		return 1;
	}
	LIBSSH2_SFTP_HANDLE *sftp_dir = libssh2_sftp_opendir(sftp_session, dirpath);
	if (sftp_dir == NULL) {
		cerr << "couldn't open dir" << endl;
		return 1;
	}

	LIBSSH2_SFTP_ATTRIBUTES sftp_attr;

	char *name = new char[512];
	char *mem = new char[512];

	q = c.query("CREATE TABLE IF NOT EXISTS directoriestofiles (did INT, fid INT);");
	q.execute();

	int rc = 0;
	while ((rc = libssh2_sftp_readdir_ex(sftp_dir, mem, 512, name, 512, &sftp_attr)) > 0) {
		if ((sftp_attr.permissions & 0100000L) != 0) {
			cout << name << endl;

			Query q = c.query("CREATE TABLE IF NOT EXISTS files (fid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, name VARCHAR(256), ch BIGINT, uid SMALLINT, gid SMALLINT, perm SMALLINT, size BIGINT)");

			q.execute();

			std::stringstream fs;

			fs << "INSERT INTO files VALUES(NULL,\"" << mem << "\"," << 1 << "," << (short int) sftp_attr.uid << "," << (short int) sftp_attr.gid << "," << (short int) sftp_attr.permissions << "," << (long int) sftp_attr.filesize << ");";

			cerr << "query: " << fs.str() << endl;

			q = c.query(fs.str());

			int fid = q.execute().insert_id();
			
			stringstream qs;
			qs << "INSERT INTO directoriestofiles VALUES(" << did << "," << fid << ");";
			cerr << qs.str() << endl;
			q.execute(qs.str());

			stringstream full_name;
			full_name << dirpath << "/" << mem;

			insert_blocks(fid, full_name.str().c_str());
		}
	}

	libssh2_sftp_closedir(sftp_dir);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}


int print_query(const char *q) {
	Query q2 = c.query(q);

	StoreQueryResult r = q2.store();
	StoreQueryResult::iterator it;
	
	for (it = r.begin(); it != r.end(); it++) {
		Row r = *it;
		cout << " " << r[0] << endl;
	}
	cout << endl;

	return 0;
}

int
run_client(int cid)
{
	stringstream qs;
	struct sockaddr_in sin;
	char *username, *password;
	StoreQueryResult r;
	StoreQueryResult::iterator it;
	try {
		qs << "SELECT * FROM clients WHERE cid=" << cid << ";";
		Query q = c.query(qs.str());

		r = q.store();
		
		for (it = r.begin(); it != r.end(); it++) {
			Row r = *it;
			cout << " " << r[0] << " " << r[1] << " " << r[2] << " " << r[3] << endl;
			sin.sin_addr.s_addr = r[1];
			username = strdup(r[2]);
			password = strdup(r[3]);
		}
	}
	catch (const mysqlpp::BadQuery& er) {
		cerr << "no clients found: " << er.what() << endl;
		return 1;
	}

	libssh2_init(0);

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);

	if (connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
		cerr << "failed to connect" << endl;
		return 1;
	}

	ssh_session = libssh2_session_init();

	libssh2_session_set_blocking(ssh_session, 1);

	if (libssh2_session_handshake(ssh_session, sock)) {
		cerr << "handshake failed" << endl;
		return 1;
	}

	if (libssh2_userauth_password(ssh_session, username, password)) {
		cerr << "couldn't authenticate user" << endl;
		return 1;
	}

	Query q = c.query("CREATE TABLE IF NOT EXISTS directories (did INT AUTO_INCREMENT PRIMARY KEY NOT NULL, path VARCHAR(4096), uid SMALLINT, gid SMALLINT, perm SMALLINT);");
	q.execute();

	q = c.query("CREATE TABLE IF NOT EXISTS clientstodirectories (cid INT, did INT);");
	q.execute();


	try {
		qs.clear();
		qs.str(string());
		qs << "SELECT * FROM clientstodirectories WHERE cid=" << cid << ";";
		cerr << qs.str() << endl;
		q = c.query(qs.str());
		
		r = q.store();

		for (it = r.begin(); it != r.end(); it++) {
			Row r = *it;
			ssh_ftw(r[1]);
		}
	}
	catch (const mysqlpp::BadQuery& er) {
		cerr << "no clients found: " << er.what() << endl;
		return 1;
	}

	libssh2_session_disconnect(ssh_session, "bye");

	libssh2_session_free(ssh_session);

	libssh2_exit();

	return 0;
}
int
run()
{
	c.connect("crowdify", "localhost", "root", "root");


	create_test_data();

	stringstream qs;
	qs.clear();
	qs.str(string());
	qs << "SELECT * FROM clients;";
	Query q = c.query(qs.str());

	StoreQueryResult r = q.store();
	StoreQueryResult::iterator it;
	
	struct sockaddr_in sin;
	char *username, *password;
	for (it = r.begin(); it != r.end(); it++) {
		Row r = *it;
		run_client(r[0]);
	}

	print_query("select * from blocks;");
	print_query("select * from files;");
	print_query("select * from clients;");
	print_query("select * from filestoblocks;");
	print_query("select * from directoriestofiles;");
	print_query("select * from clientstodirectories;");

	return 0;
}

int
main()
{
	run();
	return 0;
}
