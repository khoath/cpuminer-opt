#include <memory.h>
#include <stdlib.h>

#include "miner.h"
#include "algo-gate-api.h"
#include "hodl.h"
#include "hodl-wolf.h"

static struct work hodl_work;

pthread_barrier_t hodl_barrier;

// other algos that use a scratchbuf allocate one per miner thread
// and define it locally.
// Hodl only needs one scratchbuf total but the allocation is done in the
// miner thread, so use a flag.
// All miner threads must point to the same buffer. To do this save a copy
// of the allocated buffer pointer to use instead of malloc.

unsigned char *hodl_scratchbuf = NULL;
bool hodl_scratchbuf_allocated = false;

// get scratchbuf pointer, alloc buf if not yet done
bool hodl_alloc_scratchbuf( unsigned char** scratchbuf )
{
  if ( !hodl_scratchbuf_allocated )
  {
      hodl_scratchbuf = (unsigned char*)malloc( 1 << 30 );
      hodl_scratchbuf_allocated = ( hodl_scratchbuf != NULL );
  }
  *scratchbuf = hodl_scratchbuf;
  return ( *scratchbuf != NULL );
}

void hodl_set_target( struct work* work, double diff )
{
     diff_to_target(work->target, diff / 8388608.0 );
}

char *hodl_le_build_stratum_request( char* req, struct work* work,
                                     struct stratum_ctx *sctx ) 
{
   const int ntime_i      = 17;
   const int nstartloc_i  = 20;
   const int nfinalcalc_i = 21;
   uint32_t ntime,       nonce,       nstartloc,       nfinalcalc;
   char     ntimestr[9], noncestr[9], nstartlocstr[9], nfinalcalcstr[9];
   unsigned char *xnonce2str;

   le32enc( &ntime, work->data[ ntime_i ] );
   le32enc( &nonce, *( algo_gate.get_nonceptr( work->data ) ) );
   bin2hex( ntimestr, (char*)(&ntime), sizeof(uint32_t) );
   bin2hex( noncestr, (char*)(&nonce), sizeof(uint32_t) );
   xnonce2str = abin2hex(work->xnonce2, work->xnonce2_len );
   le32enc( &nstartloc,  work->data[ nstartloc_i ] );
   le32enc( &nfinalcalc, work->data[ nfinalcalc_i ] );
   bin2hex( nstartlocstr,  (char*)(&nstartloc),  sizeof(uint32_t) );
   bin2hex( nfinalcalcstr, (char*)(&nfinalcalc), sizeof(uint32_t) );
   sprintf( req, "{\"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"], \"id\":4}",
           rpc_user, work->job_id, xnonce2str, ntimestr, noncestr,
           nstartlocstr, nfinalcalcstr );
   free( xnonce2str );
}

void hodl_build_extraheader( struct work* work, struct stratum_ctx *sctx )
{
   const int ntime_i = 17;
   const int nbits_i = 18;
   work->data[ ntime_i ] = le32dec( sctx->job.ntime );
   work->data[ nbits_i ] = le32dec( sctx->job.nbits );
   work->data[22] = 0x80000000;
   work->data[31] = 0x00000280;
}

// called only by thread 0, saves a backup of g_work
void hodl_init_nonce( struct work* work, struct work* g_work)
{
   const int nonce_i = 19;
   const int wkcmp_sz = 76;   // nonce_index * sizeof(uint32_t)
   if ( memcmp( hodl_work.data, g_work->data, wkcmp_sz ) )
   {
      work_free( &hodl_work );
      work_copy( &hodl_work, g_work );
   }
   hodl_work.data[ nonce_i ] = ( clock() + rand() ) % 9999;
}

// called by every thread, copies the backup to each thread's work.
void hodl_resync_threads( struct work* work )
{
   const int nonce_i = 19;
   const int wkcmp_sz = 76;   // nonce_index * sizeof(uint32_t)
   pthread_barrier_wait( &hodl_barrier );
   if ( memcmp( work->data, hodl_work.data, wkcmp_sz ) )
   {
      work_free( work );
      work_copy( work, &hodl_work );
   }
   work->data[ nonce_i ] = swab32( hodl_work.data[ nonce_i ] );
}

bool hodl_do_this_thread( int thr_id )
{
  return ( thr_id == 0 );
}

// Non AES hodl fails to compile in mingw so is disabled on Windows.

int hodl_scanhash( int thr_id, struct work* work, uint32_t max_nonce,
                   uint64_t *hashes_done, unsigned char *scratchbuf )
{
#ifdef NO_AES_NI
#if (!(defined(_WIN64) || defined(__WINDOWS__)))
  GetPsuedoRandomData( scratchbuf, work->data, thr_id );
  scanhash_hodl( thr_id, work, max_nonce, hashes_done, scratchbuf );
#endif
#else
  GenRandomGarbage( scratchbuf, work->data, thr_id );
  scanhash_hodl_wolf( thr_id, work, max_nonce, hashes_done, scratchbuf );
#endif
}

bool register_hodl_algo( algo_gate_t* gate )
{
#if defined(NO_AES_NI) && (defined(_WIN64) || defined(__WINDOWS))
  algo_not_implemented();
  return false;
#else  
  pthread_barrier_init( &hodl_barrier, NULL, opt_n_threads );
  gate->aes_ni_optimized      = true;
  gate->scanhash              = (void*)&hodl_scanhash;
  gate->init_nonce            = (void*)&hodl_init_nonce;
  gate->set_target            = (void*)&hodl_set_target;
  gate->alloc_scratchbuf      = (void*)&hodl_alloc_scratchbuf;
  gate->build_stratum_request = (void*)&hodl_le_build_stratum_request;
  gate->build_extraheader     = (void*)&hodl_build_extraheader;
  gate->resync_threads        = (void*)&hodl_resync_threads;
  gate->do_this_thread        = (void*)&hodl_do_this_thread;
  return true;
#endif
}


