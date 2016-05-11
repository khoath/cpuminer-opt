#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "miner.h"

/////////////////////////////
////
////    NEW FEATURE: algo_gate
////
////    algos define targets for their common functions
////    and define a function for miner-thread to call to register
////    their targets. miner thread builds the gate, and array of structs
////    of function pointers, by calling each algo's register function.
//
//
// 
//    So you want to add an algo. Well it is a little easier now.
//    Look at existing algos for guidance.
//
//    1. Define the algo, miner.h, previously in cpu-miner.c
//
//    2.Define custom versions of the mandatory function for the new algo.
//
//    3. Next look through the list of unsafe functions to determine
//    if any apply to the new algo. If so they must also be defined.
//
//    4. Look through the list of safe functions to see if any apply
//    to the new algo. If so look at the null instance of the function
//    to see if it satisfies its needs.
//
//    5. If any of the default safe functions are not fit for the new algo
//    a custom function will have to be defined.
//
//    6. Determine if other non existant functions are required.
//    That is determined by the need to add code in cpu-miner.c
//    that applies only to the new algo. That is forbidden. All
//    algo specific code must be in theh algo's file.
//
//    7. If new functions need to be added to the gate add the type
//    to the structure, declare a null instance in this file and define
//    it in algo-gate-api.c. It must be a safe optional function so the null
//    instance must return a success code and otherwise do nothing.
//
//    8. When all the custom functions are defined write a registration
//    function to initialze the gate's function pointers with the custom
//    functions. It is not necessary to initialze safe optional null
//    instances as they are defined by default, or unsafe functions that
//    are not needed by the algo.
//
//    9. Add an case entry to the switch/case in function register_gate
//    in file algo-gate-api.c for the new algo.
//
//    10 If a new function type was defined add an entry to ini talgo_gate
//    to initialize the new function to its null instance described in step 7.
//
//    11. If the new algo has aliases add them to the alias array in
//    algo-gate-api.c 
//
//    12. Include algo-gate-api.h and miner.h inthe algo's source file.
//
//    13. Inlude any other algo source files required by the new algo.
//
//    14. Done, compile and run. 


// declare some function pointers
// mandatory functions require a custom function specific to the algo
// be defined. 
// otherwise the null instance will return a fail code.
// Optional functions may not be required for certain algos or the null
// instance provides a safe default. If the default is suitable for
//  an algo it is not necessary to define a custom function.
//

typedef struct
{
//migrate to use work instead of pdata & ptarget, see decred for example.
// mandatory functions, must be overwritten
int   ( *scanhash ) ( int, struct work*, uint32_t, uint64_t*,
           unsigned char* );

// optional unsafe, must be overwritten if algo uses function
void   ( *hash )            ( void*, const void*, uint32_t ) ;
void   ( *hash_alt )        ( void*, const void*, uint32_t );
void   ( *hash_suw )        ( void*, const void* );
void   ( *init_ctx )        ();

//optional, safe to use default in most cases
bool   ( *gen_work_now )     ( int, struct work*, struct work*, uint32_t* );
void   ( *init_nonceptr )    ( struct work*, struct work* ,uint32_t**, int );
uint32_t *( *get_nonceptr )   ( uint32_t* );
void   ( *display_extra_data )      ( struct work*, uint64_t* );
void   ( *wait_for_diff )           ( struct stratum_ctx* );
int64_t ( *get_max64 )              ();
bool   ( *work_decode )             ( const struct json_t*, struct work* );
void   ( *set_target)               ( struct work*, double );
bool   ( *get_scratchbuf )          ( unsigned char** );
bool   ( *submit_getwork_result )   ( CURL*, struct work* );
void   ( *stratum_gen_work )        ( struct stratum_ctx*, struct work*, int );
void   ( *gen_merkle_root )         ( char*, struct stratum_ctx* );
void   ( *build_stratum_request )   ( char*, struct work*, 
                                       struct stratum_ctx* );
void   ( *set_work_data_endian )    ( struct work* );
void   ( *calc_network_diff )       ( struct work* );
void   ( *build_extraheader )       ( struct work*, struct stratum_ctx* );
bool   ( *prevent_dupes )           ( uint32_t*, struct work*,
                                       struct stratum_ctx*, int );
void   ( *thread_barrier_init )     ();
void   ( *thread_barrier_wait )     ();
void   ( *backup_work_data )        ( struct work* );
void   ( *restore_work_data )       ( struct work* );
bool   ( *do_all_threads )          ();
void   ( *get_pseudo_random_data )  ( struct work*, char*, int );
json_t* (*longpoll_rpc_call)        ( CURL*, int*, char* );
bool   ( *stratum_handle_response ) ( json_t* );
int   data_size;
bool  aes_ni_optimized;

} algo_gate_t;

extern algo_gate_t algo_gate;

// Declare null instances, default for many gate functions
void do_nothing();
bool return_true();
bool return_false();
void *return_null();
void algo_not_tested();

// allways returns failure
int null_scanhash ( int thr_id, struct work* work, uint32_t max_nonce,
                    uint64_t *hashes_done, unsigned char* scratchbuf );

// displays warning
void null_hash     ( void *output, const void *pdata, uint32_t len );
void null_hash_alt ( void *output, const void *pdata, uint32_t len );
void null_hash_suw ( void *output, const void *pdata );

// optional safe

bool std_gen_work_now( int thr_id, struct work *work, struct work *g_work, 
                       uint32_t *nonceptr );
bool jr2_gen_work_now( int thr_id, struct work *work, struct work *g_work,
                       uint32_t *nonceptr );

uint32_t *std_get_nonceptr( uint32_t *work_data );
uint32_t *jr2_get_nonceptr( uint32_t *work_data );

void std_init_nonceptr ( struct work* work, struct work* g_work,
                         uint32_t **nonceptr, int thr_id );
void jr2_init_nonceptr ( struct work* work, struct work* g_work,
                         uint32_t **nonceptr, int thr_id );

void std_stratum_gen_work( struct stratum_ctx *sctx, struct work *work,
                           int thr_id );
void jr2_stratum_gen_work( struct stratum_ctx *sctx, struct work *work );

// default
void   sha256d_gen_merkle_root( char* merkle_root, struct stratum_ctx* sctx );
void   SHA256_gen_merkle_root ( char* merkle_root, struct stratum_ctx* sctx );

// pick your favorite or define your own
int64_t get_max64_0x1fffffLL(); // default
int64_t get_max64_0x40LL();
int64_t get_max64_0x3ffff();
int64_t get_max64_0x3fffffLL();
int64_t get_max64_0x1ffff();

void   std_set_target ( struct work* work, double job_diff );
void   scrypt_set_target( struct work* work, double job_diff );

// default
bool std_work_decode( const json_t *val, struct work *work);
bool jr2_work_decode( const json_t *val, struct work *work);

bool std_submit_getwork_result( CURL *curl, struct work *work );
bool jr2_submit_getwork_result( CURL *curl, struct work *work );

// default
void build_stratum_request_le( char *req, struct work *work );
void build_stratum_request_be( char *req, struct work *work );
void jr2_build_stratum_request( char* req, struct work* work );

// default
void std_set_work_data_endian( struct work *work );
void swab_work_data( struct work *work );

void   std_calc_network_diff( struct work* work );

void   std_build_extraheader( struct work* work, struct stratum_ctx* sctx );

// This is the default, if you need null do it yourself.
void    std_init_nonceptr ( struct work* work, struct work* g_work,
                            uint32_t **nonceptr, int thr_id );
void    jr2_init_nonceptr ( struct work* work, struct work* g_work,
                            uint32_t **nonceptr, int thr_id );

json_t* std_longpoll_rpc_call( CURL *curl, int *err, char* lp_url );
json_t* jr2_longpoll_rpc_call( CURL *curl, int *err );

bool std_stratum_handle_response( json_t *val );
bool jr2_stratum_handle_response( json_t *val );

// Gate admin functions
bool register_algo_gate( int algo, algo_gate_t *gate );

// The register functions for all the algos can be declared here to reduce
// compiler warnings but that's just more work for devs adding new algos.
bool register_algo( algo_gate_t *gate );

// called by algos that use rpc2
bool register_json_rpc2( algo_gate_t *gate );

// use this to call the hash function of an algo directly, ie util.c test.
void exec_hash_function( int algo, void *output, const void *pdata );

void get_algo_alias( char** algo_or_alias );

