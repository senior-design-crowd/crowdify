#include <mysql++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace mysqlpp;
using namespace std;

class ClientConfig {
	int cid, did, jid;
	Connection *con;
	public:
	ClientConfig(char *addr, char *username, char *passwd) {
		con = new Connection("crowdify", "localhost", "root", "root");

		int iaddr = inet_addr(addr);

		try {
			stringstream qs;
			qs << "SELECT * FROM clients WHERE ip=" << iaddr << ";";
			Query q = con->query(qs.str());

			StoreQueryResult r = q.store();
			StoreQueryResult::iterator it;
			for (it = r.begin(); it != r.end(); it++) {
				Row r = *it;
				cerr << "Found client: " << r[1] << endl;
				cid = r[0];
			}
		}

		catch(const mysqlpp::BadQuery& e) {
			Query q = con->query("CREATE TABLE IF NOT EXISTS clients (cid INT AUTO_INCREMENT PRIMARY KEY NOT NULL, ip BIGINT, username VARCHAR(32), password VARCHAR(16));");
			q.execute();

			stringstream q2;
			q2 << "INSERT INTO TABLE clients (NULL," << iaddr << "," << username << "," << passwd << ");";
			q = con->query(q2.str());
			cid = q.execute().insert_id();
		}
	}
	int insertDirectory(char *dirpath) {
		Query q = con->query("CREATE TABLE IF NOT EXISTS directories (did INT AUTO_INCREMENT PRIMARY KEY NOT NULL, path VARCHAR(4096), uid SMALLINT, gid SMALLINT, perm SMALLINT);");
		q.execute();

		stringstream q1;
		q1 << "INSERT INTO directories VALUES(NULL,\"" << dirpath << "\",0,0,0);";
		q = con->query(q1.str());
		did = q.execute().insert_id();

		q = con->query("CREATE TABLE IF NOT EXISTS clientstodirectories (cid INT, did INT);");
		q.execute();

		stringstream q2;
		q2 << "INSERT INTO clientstodirectories VALUES(" << cid << "," << did << ");";
		q = con->query(q2.str());
		q.execute();
		
		return did;
	}
};
