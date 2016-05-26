#include <libcouchbase/couchbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int NUMBER_OF_DOCUMENTS = 10;
int flag=0;


static void storage_callback(lcb_t instance, const void *cookie, lcb_storage_t op, 
   lcb_error_t err, const lcb_store_resp_t *resp)
{
//  printf("Stored %.*s\n", (int)resp->v.v0.nkey, resp->v.v0.key);
    printf(".");

}

static void get_callback(lcb_t instance, const void *cookie, lcb_error_t err, 
   const lcb_get_resp_t *resp)
{
    //  If you can't read from active, read from replica.
    if (err != LCB_SUCCESS && flag == 0 ) {
        printf("REPLICA\t");
        printf("Failed to read from active copy: %s\n", lcb_strerror(instance,err));
        fprintf(stderr, "Reading from REPLICA. Failed to read from active copy: %s\n", lcb_strerror(instance,err));

        lcb_get_replica_cmd_t rcmd = {0};
        const lcb_get_replica_cmd_t *rcmdlist = &rcmd;
        rcmd.v.v1.key = resp->v.v0.key;
        rcmd.v.v1.nkey = (int)resp->v.v0.nkey;
        rcmd.v.v1.strategy = LCB_REPLICA_FIRST;
        err = lcb_get_replica(instance, NULL, 1, &rcmdlist);
        flag=1;
        lcb_wait(instance); // get_replica_callback is invoked here

    }
    else if (err != LCB_SUCCESS && flag == 1){
        // If you fail to read from active and replica, just print the below message and the key.
        flag=0;
        printf("Failed to read from replica too: %s \n",lcb_strerror(instance,err));
        fprintf(stderr, "Failed to read from replica too: %s\n", lcb_strerror(instance, err));
        }
    else {
        // If you were able to get the key, print the key and value.
        printf("Retrieved key %.*s\t\t", (int)resp->v.v0.nkey, resp->v.v0.key);
        printf("Value is %.*s\n", (int)resp->v.v0.nbytes, resp->v.v0.bytes);
        fprintf(stderr, "Retrieved key %.*s\t value is %.*s\n", (int)resp->v.v0.nkey, resp->v.v0.key, (int)resp->v.v0.nbytes, resp->v.v0.bytes);
        flag=0;
    }

}



int main(void)
{

  // initializing
  struct lcb_create_st create_options;
  memset(&create_options, 0, sizeof create_options);
  create_options.version = 3;
  create_options.v.v3.connstr = "couchbase://10.141.110.101,10.141.110.102,10.141.110.103/testbucket";
  create_options.v.v3.passwd = "testbucket";
  lcb_error_t err;
  lcb_t instance = NULL;
  err = lcb_create(&instance, &create_options);
  if (err != LCB_SUCCESS) {
    fprintf(stderr, "Failed to create a couchbase instance: %s\n", lcb_strerror(instance,err));
    return 1;
  }
// Default timeout values and settings: http://developer.couchbase.com/documentation/server/current/sdks/java-2.2/env-config.html
//Enable details error codes:
//http://developer.couchbase.com/documentation/server/current/sdks/c-2.4/options.html
  lcb_cntl_string(instance,"detailed_errcodes","1");

  //connecting
  lcb_connect(instance);
  lcb_wait(instance);
  if ( (err = lcb_get_bootstrap_status(instance)) != LCB_SUCCESS ) {
    printf("Couldn't bootstrap. Exiting!\n");
    exit(1);
  }
  sleep(20);
  
  // installing callbacks
  lcb_set_store_callback(instance, storage_callback);
  lcb_set_get_callback(instance, get_callback);

  
  // scheduling set operations with elements that expire
  lcb_store_cmd_t scmd = { 0 };
  const lcb_store_cmd_t *scmdlist = &scmd;
/*
    INSERT, Loop through x number of keys and insert them with expiry
*/

  for( int counter=0; counter <NUMBER_OF_DOCUMENTS; counter ++){
      // Setting keys and values
      const char *doc = "{\"json\" : \"data\" }";
      char key [12];
      sprintf(key, "%s%d", "test", counter);
      scmd.v.v0.key = key;
      scmd.v.v0.nkey = strlen(key);
      scmd.v.v0.bytes = doc;
      scmd.v.v0.nbytes = strlen(doc);
//      scmd.v.v0.exptime = 300; //expiry of 5 minutes
      scmd.v.v0.operation = LCB_SET;

      err = lcb_store(instance, NULL, 1, &scmdlist);
      if (err != LCB_SUCCESS) {
        printf("Couldn't schedule storage operation!\n");
        exit(1);
      }
      lcb_wait(instance); //storage_callback is invoked here
  }


/*
    GET section
*/
  lcb_get_cmd_t gcmd = { 0 };
  const lcb_get_cmd_t *gcmdlist = &gcmd;
    for( int counter=0; counter <NUMBER_OF_DOCUMENTS; counter ++){
         char key[12];
//         sleep(3);
         sprintf(key,"%s%d", "test", counter);
         gcmd.v.v0.key = key;
         gcmd.v.v0.nkey = strlen(key);
         fprintf(stderr, "GET : %s\n", key);
//         printf("GET %s\n", key);
         err = lcb_get(instance, NULL, 1, &gcmdlist);
         lcb_wait(instance); // get_callback is invoked here
    }

  //closing the connection
  lcb_destroy(instance);
  return 0;
}
