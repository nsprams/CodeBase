#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <libcouchbase/couchbase.h>

using namespace std;

static void 
die(lcb_t instance, const char  *msg, lcb_error_t err) 
{
	fprintf(stderr, "%s. Received code 0x%X (%s)\n",
		msg, err, lcb_strerror(instance, err) );
	exit(EXIT_FAILURE);
}

static void
store_callback(lcb_t instance, const void *cookie, lcb_storage_t operation, lcb_error_t error, const lcb_store_resp_t *item)
{
	if (error == LCB_SUCCESS) {
		fprintf(stderr, "=== STORED ==== \n");
		fprintf(stderr, "KEY : %.*s\n",(int)item->v.v0.nkey, item->v.v0.key );
        //fprintf(stderr, "CAS: 0x%"PRIx64"\n", item->v.v0.cas);

	}
	else {
		die(instance, "coudn't store item", error);
	}
	(void)operation;
}

static void
get_callback(lcb_t instance, const void *cookie, lcb_error_t error, const lcb_get_resp_t *item) 
{
	if (error == LCB_SUCCESS) 
	{
		fprintf(stderr, "+++ RETRIEVED +++\n");
		fprintf(stderr, "KEY: %.*s\n", (int)item->v.v0.nkey, item->v.v0.key );
		fprintf(stderr, "VALUE: %.*s\n",(int)item->v.v0.nbytes, item->v.v0.bytes );
		//fprintf(stderr, "CAS: 0x% PRIx64 \n", item->v.v0.cas);
		fprintf(stderr, "FLAGS: 0x%x\n", item->v.v0.flags);
	}
	else 
	{
		die(instance, "couldn't retrieve", error);
	}
	(void)cookie;
}


static lcb_t 
create_instance(char* connstr, char* passwd)
{
	lcb_error_t err;
	lcb_t instance;

	struct lcb_create_st create_options;// = {0};
	memset(&create_options,0, sizeof(create_options));


//	lcb_get_cmd_t gcmd = {0};
//	const lcb_get_cmd_t *gcmdlist[1];

	create_options.version = 3;
	create_options.v.v3.connstr = connstr;
	create_options.v.v3.passwd = passwd;

	err = lcb_create(&instance, &create_options);
	if(err != LCB_SUCCESS) {
		die(instance, "Couldn't create couchbase handle.", err);
	}

	err = lcb_connect(instance);
	if(err != LCB_SUCCESS) {
		die(instance, "Couldn't schedule connect", err);
	}

	err = lcb_get_bootstrap_status(instance);
	if(err != LCB_SUCCESS) {
		die(instance, "Couldn't bootstrap from cluster", err);
	}

	lcb_set_get_callback(instance, get_callback);
	lcb_set_store_callback(instance, store_callback);

	return(instance);
}

static void
populate(lcb_t instance) {
	lcb_error_t err;
	lcb_store_cmd_t scmd;
	const lcb_store_cmd_t *scmdlist[1];

	// generate series of key and values and populate the bucket
	scmd.v.v0.operation = LCB_SET;
	scmdlist[0] = &scmd;
	string key="foo";
	string bytes="bar";

/*
	for (int i = 0; i < 1; ++i)
	{
		char *intStr = itoa(i);
		 string key_base;
		char* key_base;
		char* bytes_base;
		 string bytes_base;
		int key_length;
		int bytes_length;
		key_base.append("key");
		key_base.append(to_string(i));
		bytes_base.append("bytes");
		bytes_base.append(to_string(i));

		key_length = key_base.length();
		bytes_length = bytes_base.length();


		scmd.v.v0.key = &key_base;
		scmd.v.v0.nkey = key_length;
		scmd.v.v0.bytes = &bytes_base;
		scmd.v.v0.nbytes = bytes_length;

		scmdlist[0] = &scmd;
	}
*/

		scmd.v.v0.key = &key;
		scmd.v.v0.nkey = 3;
		scmd.v.v0.bytes = &bytes;
		scmd.v.v0.nbytes = 4;

		scmdlist[0] = &scmd;


	err = lcb_store(instance, NULL, 1, scmdlist);
	if (err != LCB_SUCCESS) {
		die(instance, "Couldn't schedule store", err);
	}
	fprintf(stderr, "Will wait for stroage operation to complete\n");
	lcb_wait(instance);

}


int main(int argc, char* argv[]) 
{

	// First initialize all connection parameters
	// define functions to create a loopable bombarding set and get ops
	lcb_t instance;

	if (argc < 2) {
		fprintf(stderr, "Usage: couchbase://hostname/bucket [password]\n");
		exit(EXIT_FAILURE);
	}

	if (argc >= 3) {
		instance = create_instance(argv[1],argv[2]);
	}
	instance = create_instance(argv[1],NULL);
	populate(instance);


}