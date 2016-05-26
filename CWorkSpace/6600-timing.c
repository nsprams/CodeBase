#include <stdio.h>
#include <libcouchbase/couchbase.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#ifdef _WIN32
#define PRIx64 "I64x"
#else
#include <inttypes.h>
#endif

// Couchbase Support- support@couchbase.com

static lcb_log_severity_t min_severity = LCB_LOG_TRACE;

static void
die(lcb_t instance, const char *msg, lcb_error_t err)
{
    fprintf(stderr, "%s. Received code 0x%X (%s)\n",
        msg, err, lcb_strerror(instance, err));
    exit(EXIT_FAILURE);
}
static void
store_callback(lcb_t instance, const void *cookie,
    lcb_storage_t operation, lcb_error_t error, const lcb_store_resp_t *item)
{
    if (error == LCB_SUCCESS) {
    } else {
        fprintf(stderr, "Store failed KEY: %.*s\n", (int)item->v.v0.nkey, item->v.v0.key);
    }
    (void)operation;
}
static void
get_callback(lcb_t instance, const void *cookie, lcb_error_t error,
    const lcb_get_resp_t *item)
{
    if (error == LCB_SUCCESS) {
    } else {
        fprintf(stderr, "Get failed KEY: %.*s\n", (int)item->v.v0.nkey, item->v.v0.key);
    }
    (void)cookie;
}
static void
log_callback(struct lcb_logprocs_st *procs, unsigned iid,
        const char *subsys, int severity, const char *srcfile,
        int srcline, const char *fmt,
        va_list ap)
{
    printf("[instance #%d] Severity %d @%s:%d (module=%s) ",
            iid, severity, srcfile, srcline, subsys);
    va_list apc;
    va_copy(apc, ap);
    vprintf(fmt, ap);
    printf("\n");
}
static struct lcb_logprocs_st logprocs = {
    .version = 0,
    .v = {
        .v0 = {
            .callback = log_callback
        }
    }
};
static void
timing_callback(lcb_t instance, const void *cookie, lcb_timeunit_t timeunit,
                lcb_U32 min, lcb_U32 max, lcb_U32 total, lcb_U32 maxtotal)
{
    FILE *out = (void*) cookie;
    int num = (float)10.0 * (float)total / ((float)maxtotal);
    fprintf(out, "[%3u - %3u]", min, max);
    switch (timeunit) {
    case LCB_TIMEUNIT_NSEC:
        fprintf(out, "ns");
        break;
    case LCB_TIMEUNIT_USEC:
        fprintf(out, "us");
        break;
    case LCB_TIMEUNIT_MSEC:
        fprintf(out, "ms");
        break;
    case LCB_TIMEUNIT_SEC:
        fprintf(out, "s");
        break;
    default:
        ;
    }

    fprintf(out, " |");
    for(int ii = 0; ii < num ; ++ii)
        fprintf(out, "#");
    fprintf(out, " - %u\n", total);
}

int main(int argc, char *argv[])
{
    int noOfDocs = 0;
    int i = 0;
    char *key;

    lcb_t instance;
    lcb_error_t err;

    struct lcb_create_st create_options = { 0 };

    lcb_store_cmd_t scmd = { 0 };
    const lcb_store_cmd_t *scmdlist[1];

    lcb_get_cmd_t gcmd = { 0 };
    const lcb_get_cmd_t *gcmdlist[1];

    create_options.version = 3;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s couchbase://host/bucket <noOfDocs> [ password ] \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    create_options.v.v3.connstr = argv[1];
    noOfDocs = atoi(argv[2]);
    if (argc >= 4)
        create_options.v.v3.passwd = argv[2];

    err = lcb_create(&instance, &create_options);
    assert (err == LCB_SUCCESS);

    err = lcb_connect(instance);
    assert (err == LCB_SUCCESS);
    lcb_wait(instance);

    err = lcb_get_bootstrap_status(instance);
    assert (err == LCB_SUCCESS);

    /*Logging stuff*/
    //err = lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_LOGGER, &logprocs);
    //assert(err == LCB_SUCCESS);

    /*Set timeout*/
    lcb_U32 tmo = 1000000;
    err = lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_OP_TIMEOUT, &tmo);
    assert(err == LCB_SUCCESS);

    /*Enable timings*/
    lcb_enable_timings(instance);

    /* Assign the handlers to be called for the operation types */
    lcb_set_get_callback(instance, get_callback);
    lcb_set_store_callback(instance, store_callback);

    for(i=0; i<noOfDocs; i++) {
        scmd.v.v0.operation = LCB_SET;
        sprintf(key, "set_test_%d", i);
        scmd.v.v0.key = key;
        scmd.v.v0.nkey = strlen(scmd.v.v0.key);
        scmd.v.v0.bytes = "bar";
        scmd.v.v0.nbytes = 3;
        scmdlist[0] = &scmd;

        err = lcb_store(instance, NULL, 1, scmdlist);
        if (err != LCB_SUCCESS) {
            die(instance, "Couldn't schedule storage operation", err);
        }
        lcb_wait(instance);
    }
    fprintf(stderr, "+---Set call timings---+\n");
    lcb_get_timings(instance, stderr, timing_callback);
    fprintf(stderr, "+----------------------+\n");
    lcb_disable_timings(instance);

    lcb_enable_timings(instance);
    for(i=0;i<noOfDocs; i++) {
        sprintf(key, "set_test_%d", i);
        gcmd.v.v0.key = key;
        gcmd.v.v0.nkey = strlen(gcmd.v.v0.key);
        gcmdlist[0] = &gcmd;
        err = lcb_get(instance, NULL, 1, gcmdlist);
        if (err != LCB_SUCCESS) {
            die(instance, "Couldn't schedule retrieval operation", err);
        }
        lcb_wait(instance);
    }
    fprintf(stderr, "+---Get call timings---+\n");
    lcb_get_timings(instance, stderr, timing_callback);
    fprintf(stderr, "+----------------------+\n");

    lcb_destroy(instance);

    return 0;
}
