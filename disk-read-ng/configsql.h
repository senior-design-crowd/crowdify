#include <mysql++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class ClientConfig {
	int cid, did, jid;
	Connection *c;
	public:
	ClientConfig(char *addr, char *username, char *passwd) {
		con->c = new Connection("crowdify", "localhost", "root", "root")

		int iaddr = inet_addr(addr);

		try {
			Query q = con->c.query(stringstream() << "SELECT * FROM clients WHERE ip=" << iaddr << ";");

			StoreResultSet r = q.store();
			StoreResultSet::iterator it;
			for (it = r.begin(); it != r.end(); it++) {
				Row r = *it;
				cerr << "Found client: " << r[1] << endl;
				cid = r[0];
			}
		}

		catch(const mysqlpp::BadQuery& e) {
			Query q = c.query("CREATE TABLE IF NOT EXISTS clients (cid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, ip BIGINT, username VARCHAR(32), password VARCHAR(16));");
			q.execute();

			q = c.query(stringstream() << "INSERT INTO TABLE clients (NULL," << iaddr << "," << username << "," << passwd << ");");
			cid = q.execute().insert_id();
		}
	}
	int insertDirectory(char *dirpath) {
		Query q = c.query("CREATE TABLE IF NOT EXISTS directories (did INT AUTO_INCREMENT PRIMARY KEY NOT NULL, path VARCHAR(4096), uid SMALLINT, gid SMALLINT, perm SMALLINT);");
		q.execute();

		q = c.query("INSERT INTO TABLE directories (NULL," << dirname << "0,0,0);";
		did = q.execute().insert_id();

		q = c.query("CREATE TABLE IF NOT EXISTS clientstodirectories (cid INT, did INT);");
		q.execute();

		q = c.query(stringstream() << "INSERT INTO clientstodirectories (" << cid << "," << did << ");");
		q.execute();
		
		return did;
	}
};
