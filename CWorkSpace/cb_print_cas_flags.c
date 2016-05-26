#include <stdio.h>
#include <libcouchbase/couchbase.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[]) 
{
	lcb_error_t err;
	lcb_t instance;
	// couchbase create struct? its a struct with options to create what? a cb cluster, bucket or something else?
	struct lcb_create_st create_options = {0};
	memset(&create_options,0,sizeof(create_options));
	// set command type, and creating a list of store commands?
	lcb_store_cmd_t scmd = {0};
	const lcb_store_cmd_t *scmdlist[1];
	// get command type, and list of get commands!
	lcb_get_cmd_t gcmd = {0};
	const lcb_get_cmd_t *gcmdlist[1];
	create_options.version = 3; // cb version 3?



	if (argc < 3) 
	{
		fprintf(stderr, "Usage: %s couchbase://host/bucket [password] \n",argv[0] );
		exit(EXIT_FAILURE);		
	}
	else {
		create_options.v.v3.connstr = argv[2];
		switch(argv[1]) {
			case 'GET':
					fprintf(stderr, "%s\n", argv[1]);
					break;
			case 'SET':
					fprintf(stderr, "%s\n", argv[1]);	
					break;
			default:
					fprintf(stderr, "%s\n", argv[1]);
		}


		if (argc >=4)
		{
			create_options.v.v3.passwd = argv[3];
		}	
	}

	
	
	//fprintf(stderr, "\n %s,%s,%s\n",create_options.v.v3.connstr,create_options.v.v3.passwd,"test" );
	/*
	create_options.version = 3;
	create_options.v.v3.connstr = "couchbase://192.168.56.101/pramod";
	create_options.v.v3.passwd = "couchbase";
	*/
	err = lcb_create(&instance, &create_options);
	if(err != LCB_SUCCESS)
	{
		die(instance, "Couldn't create couchbase handle", err);
	}

	err = lcb_connect(instance);
	if(err != LCB_SUCCESS) 
	{
		die(instance, "Couldn't schedule connection", err);
	}
	lcb_wait(instance);	// It says this will be blocking wait and won't execute in a async fashion


	err = lcb_get_bootstrap_status(instance);
	if (err != LCB_SUCCESS) 
	{
		die(instance, "Couldn't bootstrap from cluster", err);
	}
	// Assign the handlers to be called for the operations types
	lcb_set_get_callback(instance, get_callback);
	lcb_set_store_callback(instance, store_callback);
	//what are these callbacks for ? I thought callbacks are for getting results of asynchronous exectution of jobs?
	

	// what is happening in here? setting values? to the source command ? 
	//let my try to wrap this up in a loop to set a 1000 values?

	
	scmd.v.v0.operation = LCB_SET;
	scmd.v.v0.key = "foo";		scmd.v.v0.nkey = 3 ;
	scmd.v.v0.bytes = "barbeque";	scmd.v.v0.nbytes = 8;
	scmdlist[0] = &scmd;

	err = lcb_store(instance, NULL, 1, scmdlist);	// so here is where the data is getting written!!
	if (err != LCB_SUCCESS) 
	{
		die (instance, "Coudn't schedule storage operations", err);
	}

	// the store_callback is involed from lcb_wait()
	fprintf(stderr, "Will wait for storage operation to complete\n");
	lcb_wait(instance);




	//now fetch the itesms back
	gcmd.v.v0.key = "foo";
	gcmd.v.v0.nkey = 3;
	gcmdlist[0] = &gcmd;
	err = lcb_get(instance, NULL, 1, gcmdlist);
	if (err != LCB_SUCCESS) 
	{
		die(instance, "Couldn't schedule retrieval operation", err);
	}

	// get callback invoked from here.
	fprintf(stderr, "Will wait to retrieve the set item!\n");
	lcb_wait(instance);

	// close the connection handle after everything is done
	lcb_destroy(instance);
	return 0;
}