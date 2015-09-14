#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bson.h>
#include <mongoc.h>
#include <gcrypt.h>
#include <err.h>
#include <stdbool.h>

int
main(int argc, char *argv[])
{
	mongoc_init();

	mongoc_client_t *client;
	client = mongoc_client_new("mongodb://localhost:27017/");

	mongoc_collection_t *collection;
	collection = mongoc_client_get_collection(client, "crowdify", "data");

	int fd;
	fd = open("/dev/sda", O_RDONLY);

	unsigned char buf[4096];
	int i;
	for(i = 0; (read(fd, buf, sizeof(buf)) <= 0) && (i < 100000); i++) {
		printf("loop: %i\n", i);
		int hash_len;
		hash_len = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
		unsigned char hash[hash_len];

		gcry_md_hash_buffer(GCRY_MD_SHA256, hash, buf, sizeof(buf));

		bson_t *query;
		query = bson_new();

		BSON_APPEND_BINARY(query, "hash",0, hash, sizeof(hash));

		mongoc_cursor_t *cursor;
		cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

		const bson_t *result;
		if (mongoc_cursor_next(cursor, &result)) {
			warnx("found key");
			goto FOUND;
		}

		bson_t *doc;
		doc = bson_new();

		bson_oid_t oid;
		bson_oid_init(&oid, NULL);
		BSON_APPEND_OID(doc, "_id", &oid);
		BSON_APPEND_BINARY(doc, "hash",0, hash, sizeof(hash));
		BSON_APPEND_BINARY(doc, "data", 0, buf, sizeof(buf));


		bson_error_t error;
		if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
			printf("%s\n", error.message);
		}

		bson_destroy(doc);
FOUND:
		bson_destroy(query);
		memset(buf, 0, sizeof(buf));
		memset(hash, 0, sizeof(hash));
	}

	close(fd);

	mongoc_collection_destroy(collection);
	mongoc_client_destroy(client);

	return 0;
}
