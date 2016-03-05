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
		char *buf = (char *) calloc(4096, 1);
		i++;
	}

	cerr << "read exited with error code: " << rc << endl;

	::libssh2_sftp_close(fd);
}

int ssh_ftw(const char *dirpath, int (*fn) (const char*, const struct stat *, int)) {
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

	int rc = 0;
	while ((rc = libssh2_sftp_readdir_ex(sftp_dir, mem, 512, name, 512, &sftp_attr)) > 0) {
		if ((sftp_attr.permissions & 0100000L) != 0) {
			cout << name << endl;

			Query q = c.query("CREATE TABLE IF NOT EXISTS files (fid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, name VARCHAR(256), ch BIGINT, cid INT, uid SMALLINT, gid SMALLINT, perm SMALLINT, size BIGINT)");

			q.execute();

			std::stringstream fs;

			fs << "INSERT INTO files VALUES(NULL,\"" << mem << "\"," << 1 << "," << 1 << "," << (short int) sftp_attr.uid << "," << (short int) sftp_attr.gid << "," << (short int) sftp_attr.permissions << "," << (long int) sftp_attr.filesize << ");";

			cerr << "query: " << fs.str() << endl;

			q = c.query(fs.str());

			int fid = q.execute().insert_id();

			stringstream full_name;
			full_name << dirpath << "/" << mem;

			insert_blocks(fid, full_name.str().c_str());
		}
	}

	libssh2_sftp_closedir(sftp_dir);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}


int walker(const char *fpath, const struct stat *sb, int typeflag) {
	Query q = c.query("CREATE TABLE IF NOT EXISTS files (fid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, name VARCHAR(256), ch BIGINT, cid INT, uid SMALLINT, gid SMALLINT, perm SMALLINT, size BIGINT)");

	q.execute();

	std::stringstream fs;

	fs << "INSERT INTO files VALUES(NULL,\"" << basename(fpath) << "\"," << 1 << "," << 1 << "," << (short int) sb->st_uid << "," << (short int) sb->st_gid << "," << (short int) sb->st_mode << "," << (long int) sb->st_size << ");";

	cerr << "query: " << fs.str() << endl;

	q = c.query(fs.str());

	int id = q.execute().insert_id();

	insert_blocks(id, fpath);

	cerr << "current file: " << fpath << endl;

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
run()
{
	c.connect("crowdify", "localhost", "root", "sql");

	Query q = c.query("CREATE DATABASE IF NOT EXISTS crowdify");

	libssh2_init(0);

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

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

	if (libssh2_userauth_password(ssh_session, "ormris", "q4dy7w34")) {
		cerr << "couldn't authenticate user" << endl;
		return 1;
	}

	ssh_ftw("/home/ormris/src/crowdify/disk-read-ng/test", walker);

	print_query("select * from blocks;");
	print_query("select * from files;");
	print_query("select * from filestoblocks;");

	libssh2_session_disconnect(ssh_session, "bye");

	libssh2_session_free(ssh_session);

	libssh2_exit();

	return 0;
}

int
main()
{
	run();
	return 0;
}
