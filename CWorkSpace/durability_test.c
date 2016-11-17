#include <libcouchbase/couchbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int NUMBER_OF_DOCUMENTS = 100;

static void storage_callback(lcb_t instance, const void *cookie, lcb_storage_t op,
   lcb_error_t err, const lcb_store_resp_t *resp)
{
  if (err != LCB_SUCCESS) {

    fprintf(stderr, "Got error in store callback: %s\n", lcb_strerror(instance,err));
  }
  fprintf(stderr, "set done, %s\n", resp->v.v0.key );
}


static void durability_callback(lcb_t instance, const void *cookie, lcb_error_t err, const lcb_durability_resp_t *resp)
{
  if (err != LCB_SUCCESS) {
    fprintf(stderr, "Response was not successful for durability: %s\n", lcb_strerror(instance,err));
    switch (resp->version) {
      case 0:
        switch (resp->v.v0.err) {
          // Checking what is the durability response to ensure if the key was processed at all.
          // Basically this is where you can check if the key has been persisted to more than one and skip processing that key again.
          case LCB_KEY_EEXISTS:
            printf("Seems like someone modified the key already...\n");
            break;
          case LCB_ETIMEDOUT:
            printf("Timedout \n");
            printf("If persisted_master or exists_master is true, then the server is simply slow. otherwise, the key does not exist!\n");
            break;
          default:
            // printf("Got other error. [%s]", lcb_strerror(instance,err));
            printf("Got other error. [%d]", err);
            break;
        }
        break;
      default:
        printf("Response version !=0.\n");
        break;
    }
    printf("Mexits, Mpersist, #persisted, #replicated");
    printf(" (%d, %d, %d, %d) \n", resp->v.v0.exists_master,resp->v.v0.persisted_master, resp->v.v0.npersisted, resp->v.v0.nreplicated );

  }
  else {
    printf(".");
    // Durability checks successful, you may print the
    // printf("Mexits, Mpersist, #persisted, #replicated");
    // printf(" (%d, %d, %d, %d) \n", resp->v.v0.exists_master,resp->v.v0.persisted_master, resp->v.v0.npersisted, resp->v.v0.nreplicated );
  }

}



int main(void)
{
  struct lcb_create_st create_options;
  memset(&create_options, 0, sizeof create_options);
  create_options.version = 3;
  create_options.v.v3.connstr = "couchbase://10.141.95.101/default";
  // create_options.v.v3.passwd = "testbucket";
  lcb_error_t err;

  lcb_t instance = NULL;
  err = lcb_create(&instance, &create_options);
  if (err != LCB_SUCCESS) {
    fprintf(stderr, "Failed to create a couchbase instance: %s\n", lcb_strerror(instance,err));
    return 1;
  }
  // Default timeout values and settings:
  //http://developer.couchbase.com/documentation/server/current/sdks/c-2.4/options.html
  //Enable details error codes:
  lcb_cntl_string(instance,"detailed_errcodes","1");

  //Connecting
  lcb_connect(instance);
  lcb_wait(instance);
  if ( (err = lcb_get_bootstrap_status(instance)) != LCB_SUCCESS ) {
    printf("Couldn't bootstrap. Exiting!\n");
    exit(1);
  }

  // installing callbacks
  lcb_set_store_callback(instance, storage_callback);
  lcb_set_durability_callback(instance, durability_callback);

  // scheduling set operations with elements that expire
  lcb_store_cmd_t scmd = { 0 };
  const lcb_store_cmd_t *scmdlist = &scmd;
/*
    INSERT, Loop through x number of keys and insert them with expiry
*/
  for( int counter=1; counter <= NUMBER_OF_DOCUMENTS; counter++){
      // Setting keys and values
      const char *doc = "{\"json\" : \"data\" }";
      char key [12];
      sprintf(key, "%s%d", "test", counter);
      scmd.v.v0.key = key;
      scmd.v.v0.nkey = strlen(key);
      scmd.v.v0.bytes = doc;
      scmd.v.v0.nbytes = strlen(doc);
      scmd.v.v0.exptime = 300; //expiry of 5 minutes
      scmd.v.v0.operation = LCB_SET;

      err = lcb_store(instance, NULL, 1, (const lcb_store_cmd_t * const *)&scmdlist);
      if (err != LCB_SUCCESS) {
        printf("Couldn't schedule storage operation!\n");
        exit(1);
      }
      lcb_wait(instance);

      //Durability is invoked here
      lcb_durability_opts_t options;
      lcb_durability_cmd_t cmd = { 0 };
      const lcb_durability_cmd_t *cmdp = &cmd;
      cmd.v.v0.key = key;
      cmd.v.v0.nkey = strlen(key);
      options.v.v0.persist_to = 1;
      options.v.v0.replicate_to = 1;

      err = lcb_durability_poll(instance,NULL,&options,1,&cmdp);
      if (err != LCB_SUCCESS) {
        printf("couldn't schedule durability operation %s \n", lcb_strerror(instance, err));
      }
    lcb_wait(instance);
  }
  //closing the connection
  lcb_destroy(instance);
  return 0;
}
