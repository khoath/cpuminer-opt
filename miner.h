#ifndef __MINER_H__
#define __MINER_H__

#include <cpuminer-config.h>

#define USER_AGENT PACKAGE_NAME "/" PACKAGE_VERSION
#define MAX_CPUS 16

//#ifndef NO_AES_NI
 #ifndef __AES__
  #define NO_AES_NI
 #endif
//#endif

#ifdef _MSC_VER

#undef USE_ASM  /* to fix */

#ifdef NOASM
#undef USE_ASM
#endif

/* missing arch defines for msvc */
#if defined(_M_X64)
#define __i386__ 1
#define __x86_64__ 1
#elif defined(_M_X86)
#define __i386__ 1
#endif

#endif /* _MSC_VER */

#include <stdbool.h>
#include <inttypes.h>
#include <sys/time.h>

#include <pthread.h>
#include <jansson.h>
#include <curl/curl.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#ifndef min
#define min(a,b) (a>b ? b : a)
#endif
#ifndef max 
#define max(a,b) (a<b ? b : a)
#endif

//#ifdef HAVE_ALLOCA_H
//# include <alloca.h>
//#elif !defined alloca
# ifdef __GNUC__
//#  define alloca __builtin_alloca
# elif defined _AIX
#  define alloca __alloca
# elif defined _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# elif !defined HAVE_ALLOCA
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
//#endif

//#ifdef HAVE_SYSLOG_H
//#include <syslog.h>
//#define LOG_BLUE 0x10 /* unique value */
//#else
enum {
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
	/* custom notices */
	LOG_BLUE = 0x10,
};
//#endif

#include "compat.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#define WANT_BUILTIN_BSWAP
#else
#define bswap_32(x) ((((x) << 24) & 0xff000000u) | (((x) << 8) & 0x00ff0000u) \
                   | (((x) >> 8) & 0x0000ff00u) | (((x) >> 24) & 0x000000ffu))
#endif

static inline uint32_t swab32(uint32_t v)
{
#ifdef WANT_BUILTIN_BSWAP
	return __builtin_bswap32(v);
#else
	return bswap_32(v);
#endif
}

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

typedef unsigned char uchar;

#if !HAVE_DECL_BE32DEC
static inline uint32_t be32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
	    ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
}
#endif

#if !HAVE_DECL_LE32DEC
static inline uint32_t le32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
	    ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}
#endif

#if !HAVE_DECL_BE32ENC
static inline void be32enc(void *pp, uint32_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}
#endif

#if !HAVE_DECL_LE32ENC
static inline void le32enc(void *pp, uint32_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
}
#endif

#if !HAVE_DECL_LE16DEC
static inline uint16_t le16dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint16_t)(p[0]) + ((uint16_t)(p[1]) << 8));
}
#endif

#if !HAVE_DECL_LE16ENC
static inline void le16enc(void *pp, uint16_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
}
#endif

#if JANSSON_MAJOR_VERSION >= 2
#define JSON_LOADS(str, err_ptr) json_loads(str, 0, err_ptr)
#define JSON_LOADF(path, err_ptr) json_load_file(path, 0, err_ptr)
#else
#define JSON_LOADS(str, err_ptr) json_loads(str, err_ptr)
#define JSON_LOADF(path, err_ptr) json_load_file(path, err_ptr)
#endif

json_t* json_load_url(char* cfg_url, json_error_t *err);

void sha256_init(uint32_t *state);
void sha256_transform(uint32_t *state, const uint32_t *block, int swap);
void sha256d(unsigned char *hash, const unsigned char *data, int len);

#ifdef USE_ASM
#if defined(__ARM_NEON__) || defined(__i386__) || defined(__x86_64__)
#define HAVE_SHA256_4WAY 1
int sha256_use_4way();
void sha256_init_4way(uint32_t *state);
void sha256_transform_4way(uint32_t *state, const uint32_t *block, int swap);
#endif
#if defined(__x86_64__) && defined(USE_AVX2)
#define HAVE_SHA256_8WAY 1
int sha256_use_8way();
void sha256_init_8way(uint32_t *state);
void sha256_transform_8way(uint32_t *state, const uint32_t *block, int swap);
#endif
#endif

struct work;

void work_free(struct work *w);
void work_copy(struct work *dest, const struct work *src);



/* api related */
void *api_thread(void *userdata);

struct cpu_info {
	int thr_id;
	int accepted;
	int rejected;
	double khashes;
	bool has_monitoring;
	float cpu_temp;
	int cpu_fan;
	uint32_t cpu_clock;
};

struct thr_api {
	int id;
	pthread_t pth;
	struct thread_q	*q;
};
/* end of api */


#define JSON_RPC_LONGPOLL	(1 << 0)
#define JSON_RPC_QUIET_404	(1 << 1)
#define JSON_RPC_IGNOREERR  (1 << 2)

#define JSON_BUF_LEN 512

#define CL_N    "\x1B[0m"
#define CL_RED  "\x1B[31m"
#define CL_GRN  "\x1B[32m"
#define CL_YLW  "\x1B[33m"
#define CL_BLU  "\x1B[34m"
#define CL_MAG  "\x1B[35m"
#define CL_CYN  "\x1B[36m"

#define CL_BLK  "\x1B[22;30m" /* black */
#define CL_RD2  "\x1B[22;31m" /* red */
#define CL_GR2  "\x1B[22;32m" /* green */
#define CL_BRW  "\x1B[22;33m" /* brown */
#define CL_BL2  "\x1B[22;34m" /* blue */
#define CL_MA2  "\x1B[22;35m" /* magenta */
#define CL_CY2  "\x1B[22;36m" /* cyan */
#define CL_SIL  "\x1B[22;37m" /* gray */

#ifdef WIN32
#define CL_GRY  "\x1B[01;30m" /* dark gray */
#else
#define CL_GRY  "\x1B[90m"    /* dark gray selectable in putty */
#endif
#define CL_LRD  "\x1B[01;31m" /* light red */
#define CL_LGR  "\x1B[01;32m" /* light green */
#define CL_YL2  "\x1B[01;33m" /* yellow */
#define CL_LBL  "\x1B[01;34m" /* light blue */
#define CL_LMA  "\x1B[01;35m" /* light magenta */
#define CL_LCY  "\x1B[01;36m" /* light cyan */

#define CL_WHT  "\x1B[01;37m" /* white */

void   applog(int prio, const char *fmt, ...);
void   restart_threads(void);
extern json_t *json_rpc_call( CURL *curl, const char *url, const char *userpass,
                	const char *rpc_req, int *curl_err, int flags );
void   bin2hex( char *s, const unsigned char *p, size_t len );
char  *abin2hex( const unsigned char *p, size_t len );
bool   hex2bin( unsigned char *p, const char *hexstr, size_t len );
bool   jobj_binary( const json_t *obj, const char *key, void *buf,
                    size_t buflen );
int    varint_encode( unsigned char *p, uint64_t n );
size_t address_to_script( unsigned char *out, size_t outsz, const char *addr );
int    timeval_subtract( struct timeval *result, struct timeval *x,
                           struct timeval *y);
bool   fulltest( const uint32_t *hash, const uint32_t *target );
void   work_set_target( struct work* work, double diff );
double target_to_diff( uint32_t* target );
extern void diff_to_target(uint32_t *target, double diff);

double hash_target_ratio( uint32_t* hash, uint32_t* target );
void   work_set_target_ratio( struct work* work, uint32_t* hash );

void   get_currentalgo( char* buf, int sz );
bool   has_aes_ni( void );
bool   has_sse2();
void   bestcpu_feature( char *outbuf, int maxsz );
void   processor_id ( int functionnumber, int output[4] );


float cpu_temp( int core );

struct work {
	uint32_t data[48];
	uint32_t target[8];

	double targetdiff;
	double shareratio;
	double sharediff;

	int height;
	char *txs;
	char *workid;

	char *job_id;
	size_t xnonce2_len;
	unsigned char *xnonce2;
};

struct stratum_job {
	char *job_id;
	unsigned char prevhash[32];
	size_t coinbase_size;
	unsigned char *coinbase;
	unsigned char *xnonce2;
	int merkle_count;
	unsigned char **merkle;
	unsigned char version[4];
	unsigned char nbits[4];
	unsigned char ntime[4];
	bool clean;
	double diff;
};

struct stratum_ctx {
	char *url;

	CURL *curl;
	char *curl_url;
	char curl_err_str[CURL_ERROR_SIZE];
	curl_socket_t sock;
	size_t sockbuf_size;
	char *sockbuf;
	pthread_mutex_t sock_lock;

	double next_diff;
	double sharediff;

	char *session_id;
	size_t xnonce1_size;
	unsigned char *xnonce1;
	size_t xnonce2_size;
	struct stratum_job job;
	struct work work;
	pthread_mutex_t work_lock;

	int bloc_height;
};

bool stratum_socket_full(struct stratum_ctx *sctx, int timeout);
bool stratum_send_line(struct stratum_ctx *sctx, char *s);
char *stratum_recv_line(struct stratum_ctx *sctx);
bool stratum_connect(struct stratum_ctx *sctx, const char *url);
void stratum_disconnect(struct stratum_ctx *sctx);
bool stratum_subscribe(struct stratum_ctx *sctx);
bool stratum_authorize(struct stratum_ctx *sctx, const char *user, const char *pass);
bool stratum_handle_method(struct stratum_ctx *sctx, const char *s);

/* rpc 2.0 (xmr) */


extern bool jsonrpc_2;
extern bool aes_ni_supported;
extern char rpc2_id[64];
extern char *rpc2_blob;
extern size_t rpc2_bloblen;
extern uint32_t rpc2_target;
extern char *rpc2_job_id;
extern char *rpc_user;


json_t *json_rpc2_call(CURL *curl, const char *url, const char *userpass, const char *rpc_req, int *curl_err, int flags);
bool rpc2_login(CURL *curl);
bool rpc2_login_decode(const json_t *val);
bool rpc2_workio_login(CURL *curl);
bool rpc2_stratum_job(struct stratum_ctx *sctx, json_t *params);
bool rpc2_job_decode(const json_t *job, struct work *work);

struct thread_q;

struct thread_q *tq_new(void);
void tq_free(struct thread_q *tq);
bool tq_push(struct thread_q *tq, void *data);
void *tq_pop(struct thread_q *tq, const struct timespec *abstime);
void tq_freeze(struct thread_q *tq);
void tq_thaw(struct thread_q *tq);

void parse_arg(int key, char *arg);
void parse_config(json_t *config, char *ref);
void proper_exit(int reason);

void applog_hash(void *hash);
void format_hashrate(double hashrate, char *output);
void print_hash_tests(void);



struct thr_info {
        int id;
        pthread_t pth;
        pthread_attr_t attr;
        struct thread_q *q;
        struct cpu_info cpu;
};

struct work_restart {
        volatile uint8_t restart;
        char padding[128 - sizeof(uint8_t)];
};

enum workio_commands {
        WC_GET_WORK,
        WC_SUBMIT_WORK,
};

struct workio_cmd {
        enum workio_commands cmd;
        struct thr_info *thr;
        union {
                struct work *work;
        } u;
};

uint32_t* get_stratum_job_ntime();

enum algos {
        ALGO_NULL,
        ALGO_ARGON2,
        ALGO_AXIOM,       
        ALGO_BASTION,
        ALGO_BLAKE,       
        ALGO_BLAKECOIN,   
        ALGO_BLAKE2S,     
        ALGO_BMW,        
        ALGO_C11,         
        ALGO_CRYPTOLIGHT, 
        ALGO_CRYPTONIGHT, 
        ALGO_DECRED,
        ALGO_DROP,        
        ALGO_FRESH,       
        ALGO_GROESTL,     
        ALGO_HEAVY,
        ALGO_HMQ1725,
        ALGO_HODL,
        ALGO_KECCAK,
        ALGO_LUFFA,       
        ALGO_LYRA2RE,       
        ALGO_LYRA2REV2,   
        ALGO_M7M,
        ALGO_MYR_GR,      
        ALGO_NEOSCRYPT,
        ALGO_NIST5,       
        ALGO_PENTABLAKE,  
        ALGO_PLUCK,       
        ALGO_QUARK,
        ALGO_QUBIT,       
        ALGO_SCRYPT,
        ALGO_SCRYPTJANE,
        ALGO_SHA256D,
        ALGO_SHAVITE3,    
        ALGO_SKEIN,       
        ALGO_SKEIN2,      
        ALGO_S3,          
        ALGO_VANILLA,
        ALGO_X11,
        ALGO_X11EVO,         
        ALGO_X11GOST,
        ALGO_X13,         
        ALGO_X14,        
        ALGO_X15,       
        ALGO_X17,
        ALGO_YESCRYPT,
        ALGO_ZR5,
        ALGO_COUNT
};
static const char *algo_names[] = {
        NULL,
        "argon2",
        "axiom",
        "bastion",
        "blake",
        "blakecoin",
        "blake2s",
        "bmw",
        "c11",
        "cryptolight",
        "cryptonight",
        "decred",
        "drop",
        "fresh",
        "groestl",
        "heavy",
        "hmq1725",
        "hodl",
        "keccak",
        "luffa",
        "lyra2re",
        "lyra2rev2",
        "m7m",
        "myr-gr",
        "neoscrypt",
        "nist5",
        "pentablake",
        "pluck",
        "quark",
        "qubit",
        "scrypt",
        "scryptjane",
        "sha256d",
        "shavite3",
        "skein",
        "skein2",
        "s3",
        "vanilla",
        "x11",
        "x11evo",
        "x11gost",
        "x13",
        "x14",
        "x15",
        "x17",
        "yescrypt",
        "zr5",
        "\0"
};

extern enum algos opt_algo;
extern bool opt_debug;
extern bool opt_debug_diff;
extern bool opt_benchmark;
extern bool opt_protocol;
extern bool opt_showdiff;
extern bool opt_extranonce;
extern bool opt_quiet;
extern bool opt_redirect;
extern int opt_timeout;
extern bool want_longpoll;
extern bool have_longpoll;
extern bool have_gbt;
extern bool allow_getwork;
extern bool want_stratum;
extern bool have_stratum;
extern char *opt_cert;
extern char *opt_proxy;
extern long opt_proxy_type;
extern bool use_syslog;
extern bool use_colors;
extern pthread_mutex_t applog_lock;
extern struct thr_info *thr_info;
extern int longpoll_thr_id;
extern int stratum_thr_id;
extern int api_thr_id;
extern int opt_n_threads;
extern struct work_restart *work_restart;
extern uint32_t opt_work_size;
extern double *thr_hashrates;
extern double global_hashrate;
extern double stratum_diff;
extern double net_diff;
extern double net_hashrate;
extern int opt_pluck_n;
extern int opt_scrypt_n;
extern double opt_diff_factor;
extern unsigned int opt_nfactor;
extern bool opt_randomize;
extern bool allow_mininginfo;
extern time_t g_work_time;

extern pthread_mutex_t rpc2_job_lock;
extern pthread_mutex_t rpc2_login_lock;
extern pthread_mutex_t applog_lock;
extern pthread_mutex_t stats_lock;


static char const usage[] = "\
Usage: " PACKAGE_NAME " [OPTIONS]\n\
Options:\n\
  -a, --algo=ALGO       specify the algorithm to use\n\
                          argon2\n\
                          axiom        Shabal-256 MemoHash\n\
                          bastion\n\
                          blake        Blake-256 (SFR)\n\
                          blakecoin    blake256r8\n\
                          blake2s      Blake-2 S\n\
                          bmw          BMW 256\n\
                          c11          flax\n\
                          cryptolight  Cryptonight-light\n\
                          cryptonight  Monero\n\
                          decred\n\
                          drop         Dropcoin\n\
                          fresh        Fresh\n\
                          groestl      groestl\n\
                          hmq1725      Espers\n\
                          hodl         hodlcoin\n\
                          heavy        Heavy\n\
                          keccak       Keccak\n\
                          luffa        Luffa\n\
                          lyra2re      lyra2\n\
                          lyra2rev2    lyrav2\n\
                          m7m\n\
                          myr-gr       Myriad-Groestl\n\
                          neoscrypt    NeoScrypt(128, 2, 1)\n\
                          nist5        Nist5\n\
                          pluck        Pluck:128 (Supcoin)\n\
                          pentablake   Pentablake\n\
                          quark        Quark\n\
                          qubit        Qubit\n\
                          scrypt       scrypt(1024, 1, 1) (default)\n\
                          scryptjane\n\
                          scrypt:N     scrypt(N, 1, 1)\n\
                          sha256d      SHA-256d\n\
                          shavite3     Shavite3\n\
                          skein        Skein+Sha (Skeincoin)\n\
                          skein2       Double Skein (Woodcoin)\n\
                          s3           S3\n\
                          vanilla      blake256r8vnl (VCash)\n\
                          x11          X11\n\
                          x11evo       Revolvercoin\n\
                          x11gost      sib (SibCoin)\n\
                          x13          X13\n\
                          x14          X14\n\
                          x15          X15\n\
                          x17\n\
                          yescrypt\n\
                          zr5          Ziftr\n\
  -o, --url=URL         URL of mining server\n\
  -O, --userpass=U:P    username:password pair for mining server\n\
  -u, --user=USERNAME   username for mining server\n\
  -p, --pass=PASSWORD   password for mining server\n\
      --cert=FILE       certificate for mining server using SSL\n\
  -x, --proxy=[PROTOCOL://]HOST[:PORT]  connect through a proxy\n\
  -t, --threads=N       number of miner threads (default: number of processors)\n\
  -r, --retries=N       number of times to retry if a network call fails\n\
                          (default: retry indefinitely)\n\
  -R, --retry-pause=N   time to pause between retries, in seconds (default: 30)\n\
      --time-limit=N    maximum time [s] to mine before exiting the program.\n\
  -T, --timeout=N       timeout for long poll and stratum (default: 300 seconds)\n\
  -s, --scantime=N      upper bound on time spent scanning current work when\n\
                          long polling is unavailable, in seconds (default: 5)\n\
      --randomize       Randomize scan range start to reduce duplicates\n\
  -f, --diff-factor     Divide req. difficulty by this factor (std is 1.0)\n\
  -m, --diff-multiplier Multiply difficulty by this factor (std is 1.0)\n\
  -n, --nfactor         neoscrypt N-Factor\n\
      --coinbase-addr=ADDR  payout address for solo mining\n\
      --coinbase-sig=TEXT  data to insert in the coinbase when possible\n\
      --no-longpoll     disable long polling support\n\
      --no-getwork      disable getwork support\n\
      --no-gbt          disable getblocktemplate support\n\
      --no-stratum      disable X-Stratum support\n\
      --no-extranonce   disable Stratum extranonce support\n\
      --no-redirect     ignore requests to change the URL of the mining server\n\
  -q, --quiet           disable per-thread hashmeter output\n\
      --no-color        disable colored output\n\
  -D, --debug           enable debug output\n\
  -P, --protocol-dump   verbose dump of protocol-level activities\n"
#ifdef HAVE_SYSLOG_H
"\
  -S, --syslog          use system log for output messages\n"
#endif
"\
  -B, --background      run the miner in the background\n\
      --benchmark       run in offline benchmark mode\n\
      --cputest         debug hashes from cpu algorithms\n\
      --cpu-affinity    set process affinity to cpu core(s), mask 0x3 for cores 0 and 1\n\
      --cpu-priority    set process priority (default: 0 idle, 2 normal to 5 highest)\n\
  -b, --api-bind        IP/Port for the miner API (default: 127.0.0.1:4048)\n\
      --api-remote      Allow remote control\n\
      --max-temp=N      Only mine if cpu temp is less than specified value (linux)\n\
      --max-rate=N[KMG] Only mine if net hashrate is less than specified value\n\
      --max-diff=N      Only mine if net difficulty is less than specified value\n\
  -c, --config=FILE     load a JSON-format configuration file\n\
  -V, --version         display version information and exit\n\
  -h, --help            display this help text and exit\n\
";

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
struct option {
        const char *name;
        int has_arg;
        int *flag;
        int val;
};
#endif


static struct option const options[] = {
        { "algo", 1, NULL, 'a' },
        { "api-bind", 1, NULL, 'b' },
        { "api-remote", 0, NULL, 1030 },
        { "background", 0, NULL, 'B' },
        { "benchmark", 0, NULL, 1005 },
        { "cputest", 0, NULL, 1006 },
        { "cert", 1, NULL, 1001 },
        { "coinbase-addr", 1, NULL, 1016 },
        { "coinbase-sig", 1, NULL, 1015 },
        { "config", 1, NULL, 'c' },
        { "cpu-affinity", 1, NULL, 1020 },
        { "cpu-priority", 1, NULL, 1021 },
        { "no-color", 0, NULL, 1002 },
        { "debug", 0, NULL, 'D' },
        { "diff-factor", 1, NULL, 'f' },
        { "diff", 1, NULL, 'f' }, // deprecated (alias)
        { "diff-multiplier", 1, NULL, 'm' },
        { "help", 0, NULL, 'h' },
        { "nfactor", 1, NULL, 'n' },
        { "no-gbt", 0, NULL, 1011 },
        { "no-getwork", 0, NULL, 1010 },
        { "no-longpoll", 0, NULL, 1003 },
        { "no-redirect", 0, NULL, 1009 },
        { "no-stratum", 0, NULL, 1007 },
        { "no-extranonce", 0, NULL, 1012 },
        { "max-temp", 1, NULL, 1060 },
        { "max-diff", 1, NULL, 1061 },
        { "max-rate", 1, NULL, 1062 },
        { "pass", 1, NULL, 'p' },
        { "protocol", 0, NULL, 'P' },
        { "protocol-dump", 0, NULL, 'P' },
        { "proxy", 1, NULL, 'x' },
        { "quiet", 0, NULL, 'q' },
        { "retries", 1, NULL, 'r' },
        { "retry-pause", 1, NULL, 'R' },
        { "randomize", 0, NULL, 1024 },
        { "scantime", 1, NULL, 's' },
        { "show-diff", 0, NULL, 1013 },
#ifdef HAVE_SYSLOG_H
        { "syslog", 0, NULL, 'S' },
#endif
        { "time-limit", 1, NULL, 1008 },
        { "threads", 1, NULL, 't' },
        { "timeout", 1, NULL, 'T' },
        { "url", 1, NULL, 'o' },
        { "user", 1, NULL, 'u' },
        { "userpass", 1, NULL, 'O' },
        { "version", 0, NULL, 'V' },
        { 0, 0, 0, 0 }
};


#endif /* __MINER_H__ */

