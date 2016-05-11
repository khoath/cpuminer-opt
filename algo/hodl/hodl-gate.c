#include <memory.h>
#include <stdlib.h>

#include "miner.h"
#include "algo-gate-api.h"
#include "hodl.h"
#include "hodl-wolf.h"


void hodl_set_target( struct work* work, double diff )
{
     diff_to_target(work->target, diff / 8388608.0 );
}

// other algos that use a scratchbuf allocate one per miner thread
// and define it locally.
// Hodl only needs one scratchbuf total but the allocation is done in the
// miner thread, so use a flag.
// All miner threads must point to the same buffer. To do this save a copy
// of the allocated buffer pointer to use instead of malloc.

unsigned char *hodl_scratchbuf = NULL;
bool hodl_scratchbuf_allocated = false;

bool hodl_get_scratchbuf( unsigned char** scratchbuf )
{
  // only alloc one
  if ( !hodl_scratchbuf_allocated )
  {
      hodl_scratchbuf = (unsigned char*)malloc( 1 << 30 );
      hodl_scratchbuf_allocated = ( hodl_scratchbuf != NULL );
  }
  *scratchbuf = hodl_scratchbuf;
  return ( *scratchbuf != NULL );
}

char *hodl_build_stratum_request_le( char* req, struct work* work,
                                     struct stratum_ctx *sctx ) 
{
   unsigned char *xnonce2str;
   uint32_t ntime, nonce;
   char ntimestr[9], noncestr[9];
   uint32_t nstartloc, nfinalcalc;
   char nstartlocstr[9], nfinalcalcstr[9];

   le32enc( &ntime, work->data[17] );
   le32enc( &nonce, work->data[19] );
   bin2hex( ntimestr, (const unsigned char *)(&ntime), 4 );
   bin2hex( noncestr, (const unsigned char *)(&nonce), 4 );
   xnonce2str = abin2hex(work->xnonce2, work->xnonce2_len );
   le32enc( &nstartloc, work->data[20] );
   le32enc( &nfinalcalc, work->data[21] );
   bin2hex( nstartlocstr, (const unsigned char *)(&nstartloc), 4 );
   bin2hex( nfinalcalcstr, (const unsigned char *)(&nfinalcalc), 4 );
   sprintf( req, "{\"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"], \"id\":4}",
           rpc_user, work->job_id, xnonce2str, ntimestr, noncestr,
           nstartlocstr, nfinalcalcstr );
}

void hodl_build_extraheader( struct work* work, struct stratum_ctx *sctx )
{
        work->data[17] = le32dec(sctx->job.ntime);
        work->data[18] = le32dec(sctx->job.nbits);
        work->data[22] = 0x80000000;
        work->data[31] = 0x00000280;
}

pthread_barrier_t hodl_barrier;

void hodl_thread_barrier_init()
{
  pthread_barrier_init( &hodl_barrier, NULL, opt_n_threads);
}

void hodl_thread_barrier_wait()
{
   pthread_barrier_wait( &hodl_barrier );
}

static struct work hodl_work;
uint32_t nNonce;

uint32_t *hodl_get_nonceptr()
{
  return &nNonce;
}

void hodl_init_nonceptr()
{
   nNonce = ( clock() + rand() ) % 9999;
}

void hodl_backup_work_data( struct work* g_work )
{
  if ( memcmp( hodl_work.data, g_work->data, 76 ) )
  {
    work_free( &hodl_work );
    work_copy( &hodl_work, g_work );
  }
}

void hodl_restore_work_data( struct work* work )
{
  if ( memcmp( work->data, hodl_work.data, 76 ) )
  {
     work_free( work );
     work_copy( work, &hodl_work );
  }
  work->data[19] = swab32(nNonce);
}

bool hodl_do_all_threads ()
{
  return false;
}

void hodl_get_pseudo_random_data( struct work* work, char* scratchbuf,
                                  int thr_id )
{
#ifdef NO_AES_NI
  GetPsuedoRandomData( scratchbuf, work->data, thr_id );
#else
  GenRandomGarbage( scratchbuf, work->data, thr_id );  
#endif
}

bool register_hodl_algo ( algo_gate_t* gate )
{
#ifdef NO_AES_NI
  gate->scanhash               = (void*)&scanhash_hodl;
#else
  gate->scanhash               = (void*)&scanhash_hodl_wolf;
#endif
  gate->aes_ni_optimized       = true;
  gate->set_target             = (void*)&hodl_set_target;
  gate->get_scratchbuf         = (void*)&hodl_get_scratchbuf;
  gate->build_stratum_request  = (void*)&hodl_build_stratum_request_le;
  gate->build_extraheader      = (void*)&hodl_build_extraheader;
  gate->thread_barrier_init    = (void*)&hodl_thread_barrier_init;
  gate->thread_barrier_wait    = (void*)&hodl_thread_barrier_wait;
  gate->backup_work_data       = (void*)&hodl_backup_work_data;
  gate->restore_work_data      = (void*)&hodl_restore_work_data;
  gate->init_nonceptr          = (void*)&hodl_init_nonceptr;
  gate->get_nonceptr           = (void*)&hodl_get_nonceptr;
  gate->get_pseudo_random_data = (void*)&hodl_get_pseudo_random_data;
  gate->do_all_threads         = (void*)&hodl_do_all_threads;
  return true;
}


