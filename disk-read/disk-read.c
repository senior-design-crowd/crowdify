#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bson.h>
#include <bcon.h>
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

	mongoc_collection_t *data_collection;
	data_collection = mongoc_client_get_collection(client, "crowdify", "data");

	mongoc_collection_t *meta_collection;
	meta_collection = mongoc_client_get_collection(client, "crowdify", "meta");

	int fd;
	fd = open("/dev/sda", O_RDONLY);

	bson_t *info;
	info = bson_new();
	BSON_APPEND_UTF8(info, "block", "/dev/sda");

	bson_t child;
	BSON_APPEND_ARRAY_BEGIN(info, "blocks", &child);

	char key[100];
	unsigned char buf[4096];
	int i;
	for(i = 0; (read(fd, buf, sizeof(buf)) <= 0) && (i < 100000); i++) {
		int hash_len;
		hash_len = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
		unsigned char hash[hash_len];

		gcry_md_hash_buffer(GCRY_MD_SHA256, hash, buf, sizeof(buf));

		bson_t *query;
		query = bson_new();

		snprintf(key, sizeof(key), "%i", i);
		BSON_APPEND_BINARY(query, "hash",0, hash, sizeof(hash));
		BSON_APPEND_BINARY(&child, key, 0, hash, sizeof(hash));

		mongoc_cursor_t *cursor;
		cursor = mongoc_collection_find(data_collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

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
		if (!mongoc_collection_insert(data_collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
			printf("%s\n", error.message);
		}

		bson_destroy(doc);
FOUND:
		bson_destroy(query);
		memset(buf, 0, sizeof(buf));
		memset(hash, 0, sizeof(hash));
	}

	bson_append_array_end(info, &child);

	bson_error_t error;
	if (!mongoc_collection_insert(meta_collection, MONGOC_INSERT_NONE, info, NULL, &error)) {
		printf("%s\n", error.message);
	}

	close(fd);

	mongoc_collection_destroy(data_collection);
	mongoc_collection_destroy(meta_collection);
	mongoc_client_destroy(client);

	return 0;
}
