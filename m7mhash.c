#include "cpuminer-config.h"
#include "miner.h"

#include <gmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <sph_sha2.h>
#include <sph_keccak.h>
#include <sph_haval.h>
#include <sph_whirlpool.h>
#include <sph_ripemd.h>
#include "m7/blake2b.h"
#include "m7/poly1305.h"

#define EPSa DBL_EPSILON
#define EPS1 DBL_EPSILON
#define EPS2 3.0e-11

inline double exp_n(double xt)
{
    if(xt < -700.0)
        return 0;
    else if(xt > 700.0)
        return 1e200;
    else if(xt > -0.8e-8 && xt < 0.8e-8)
        return (1.0 + xt);
    else
        return exp(xt);
}

inline double exp_n2(double x1, double x2)
{
    double p1 = -700., p2 = -37., p3 = -0.8e-8, p4 = 0.8e-8, p5 = 37., p6 = 700.;
    double xt = x1 - x2;
    if (xt < p1+1.e-200)
        return 1.;
    else if (xt > p1 && xt < p2 + 1.e-200)
        return ( 1. - exp(xt) );
    else if (xt > p2 && xt < p3 + 1.e-200)
        return ( 1. / (1. + exp(xt)) );
    else if (xt > p3 && xt < p4)
        return ( 1. / (2. + xt) );
    else if (xt > p4 - 1.e-200 && xt < p5)
        return ( exp(-xt) / (1. + exp(-xt)) );
    else if (xt > p5 - 1.e-200 && xt < p6)
        return ( exp(-xt) );
    else if (xt > p6 - 1.e-200)
        return 0.;
}

double swit2_(double wvnmb)
{
    return pow( (5.55243*(exp_n(-0.3*wvnmb/15.762) - exp_n(-0.6*wvnmb/15.762)))*wvnmb, 0.5)
	  / 1034.66 * pow(sin(wvnmb/65.), 2.);
}


double GaussianQuad_N2(const double x1, const double x2)
{
    double s=0.0;
    double x[6], w[6];
    //gauleg(a2, b2, x, w);

    int m,j;
    double z1, z, xm, xl, pp, p3, p2, p1;
    m=3;
    xm=0.5*(x2+x1);
    xl=0.5*(x2-x1);
    for(int i=1;i<=3;i++)
    {
		z = (i == 1) ? 0.909632 : -0.0;
		z = (i == 2) ? 0.540641 : z;
	    do
	    {
			p1 = z;
			p2 = 1;
			p3 = 0;

			p3=1;
			p2=z;
			p1=((3.0 * z * z) - 1) / 2;

			p3=p2;
			p2=p1;
			p1=((5.0 * z * p2) - (2.0 * z)) / 3;

			p3=p2;
			p2=p1;
			p1=((7.0 * z * p2) - (3.0 * p3)) / 4;

			p3=p2;
			p2=p1;
			p1=((9.0 * z * p2) - (4.0 * p3)) / 5;

		    pp=5*(z*p1-p2)/(z*z-1.0);
		    z1=z;
		    z=z1-p1/pp;
	    } while (fabs(z-z1) > 3.0e-11);

	    x[i]=xm-xl*z;
	    x[5+1-i]=xm+xl*z;
	    w[i]=2.0*xl/((1.0-z*z)*pp*pp);
	    w[5+1-i]=w[i];
    }

    for(int j=1; j<=5; j++) s += w[j]*swit2_(x[j]);

    return s;
}

uint32_t sw2_(int nnounce)
{
    double wmax = ((sqrt((double)(nnounce))*(1.+EPSa))/450+100);
    return ((uint32_t)(GaussianQuad_N2(0., wmax)*(1.+EPSa)*1.e6));
}

#define BITS_PER_DIGIT 3.32192809488736234787
#define EPS (DBL_EPSILON)

#define NM7M 5
#define SW_DIVS 5
#define M7_MIDSTATE_LEN 76
int scanhash_m7m_hash(int thr_id, uint32_t *pdata, const uint32_t *ptarget,
    uint64_t max_nonce, unsigned long *hashes_done)
{
    uint32_t data[32] __attribute__((aligned(128)));
    uint32_t *data_p64 = data + (M7_MIDSTATE_LEN / sizeof(data[0]));
    uint32_t hash[8] __attribute__((aligned(32)));
    uint8_t bhash[7][64] __attribute__((aligned(32)));
    uint32_t n = pdata[19] - 1;
    uint32_t usw_, mpzscale;
    const uint32_t first_nonce = pdata[19];
    char data_str[161], hash_str[65], target_str[65];
    //uint8_t *bdata = 0;
    uint8_t bdata[8192];
    int rc = 0, i, digits;
    int bytes, nnNonce2;
    size_t p = sizeof(unsigned long), a = 64/p, b = 32/p;

    memcpy(data, pdata, 80);

    sph_sha256_context       ctx_final_sha256;

    blake2b_state            ctx_blake2b;
    sph_sha512_context       ctx_sha512;
    sph_keccak512_context    ctx_keccak;
    sph_whirlpool_context    ctx_whirlpool;
    sph_haval256_5_context   ctx_haval;
    poly1305_context         ctx_poly1305;
    sph_ripemd160_context    ctx_ripemd;

    sph_sha256_init(&ctx_final_sha256);

    blake2b_init(&ctx_blake2b);
    blake2b_update (&ctx_blake2b, data, M7_MIDSTATE_LEN);

    sph_sha512_init(&ctx_sha512);
    sph_sha512 (&ctx_sha512, data, M7_MIDSTATE_LEN);

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512 (&ctx_keccak, data, M7_MIDSTATE_LEN);

    sph_whirlpool_init(&ctx_whirlpool);
    sph_whirlpool (&ctx_whirlpool, data, M7_MIDSTATE_LEN);

    sph_haval256_5_init(&ctx_haval);
    sph_haval256_5 (&ctx_haval, data, M7_MIDSTATE_LEN);

    poly1305_init(&ctx_poly1305);
    poly1305 (&ctx_poly1305, (const unsigned char *)data, M7_MIDSTATE_LEN);

    sph_ripemd160_init(&ctx_ripemd);
    sph_ripemd160 (&ctx_ripemd, data, M7_MIDSTATE_LEN);

    blake2b_state            ctx2_blake2b;
    sph_sha512_context       ctx2_sha512;
    sph_keccak512_context    ctx2_keccak;
    sph_whirlpool_context    ctx2_whirlpool;
    sph_haval256_5_context   ctx2_haval;
    poly1305_context         ctx2_poly1305;
    sph_ripemd160_context    ctx2_ripemd;

    mpz_t magipi, magisw, product, bns0, bns1;
    mpf_t magifpi, magifpi0, mpt1, mpt2, mptmp, mpten;

    mpz_inits(magipi, magisw, bns0, bns1, NULL);
    mpz_init2(product, 512);

    mp_bitcnt_t prec0 = (long int)((int)((sqrt((double)(INT_MAX))*(1.+EPS))/9000+75)*BITS_PER_DIGIT+16);
    mpf_set_default_prec(prec0);

    mpf_init(magifpi);
    mpf_init(magifpi0);
    mpf_init(mpt1);
    mpf_init(mpt2);
    mpf_init(mptmp);
    mpf_init_set_ui(mpten, 10);
    mpf_set_str(mpt2, "0.8e3b1a9b359805c2e54c6415037f2e336893b6457f7754f6b4ae045eb6c5f2bedb26a114030846be7", 16);
    mpf_set_str(magifpi0, "0.b7bfc6837e20bdb22653f1fc419f6bc33ca80eb65b7b0246f7f3b65689560aea1a2f2fd95f254d68c", 16);

    do {
        data[19] = ++n;
        memset(bhash, 0, 7 * 64);

        ctx2_blake2b = ctx_blake2b;
        blake2b_update (&ctx2_blake2b, data_p64, 80 - M7_MIDSTATE_LEN);
        blake2b_final(&ctx2_blake2b, (void*)(bhash[0]));

        ctx2_sha512 = ctx_sha512;
        sph_sha512 (&ctx2_sha512, data_p64, 80 - M7_MIDSTATE_LEN);
        sph_sha512_close(&ctx2_sha512, (void*)(bhash[1]));

        ctx2_keccak = ctx_keccak;
        sph_keccak512 (&ctx2_keccak, data_p64, 80 - M7_MIDSTATE_LEN);
        sph_keccak512_close(&ctx2_keccak, (void*)(bhash[2]));

        ctx2_whirlpool = ctx_whirlpool;
        sph_whirlpool (&ctx2_whirlpool, data_p64, 80 - M7_MIDSTATE_LEN);
        sph_whirlpool_close(&ctx2_whirlpool, (void*)(bhash[3]));

        ctx2_haval = ctx_haval;
        sph_haval256_5 (&ctx2_haval, data_p64, 80 - M7_MIDSTATE_LEN);
        sph_haval256_5_close(&ctx2_haval, (void*)(bhash[4]));

        ctx2_poly1305 = ctx_poly1305;
        poly1305 (&ctx2_poly1305, (const unsigned char *)data_p64, 80 - M7_MIDSTATE_LEN);
        poly1305_close(&ctx2_poly1305, bhash[5]);

        ctx2_ripemd = ctx_ripemd;
        sph_ripemd160 (&ctx2_ripemd, data_p64, 80 - M7_MIDSTATE_LEN);
        sph_ripemd160_close(&ctx2_ripemd, (void*)(bhash[6]));

	mpz_import(bns0, a, -1, p, -1, 0, bhash[0]);
        mpz_set(bns1, bns0);
	mpz_set(product, bns0);
	for(int i=1; i < 7; i++){
	    mpz_import(bns0, a, -1, p, -1, 0, bhash[i]);
	    mpz_add(bns1, bns1, bns0);
            mpz_mul(product, product, bns0);
        }
        mpz_mul(product, product, bns1);

	mpz_mul(product, product, product);
        bytes = mpz_sizeinbase(product, 256);
        mpz_export((void *)bdata, NULL, -1, 1, 0, 0, product);

        sph_sha256 (&ctx_final_sha256, bdata, bytes);
        sph_sha256_close(&ctx_final_sha256, (void*)(hash));

        digits=(int)((sqrt((double)(n/2))*(1.+EPS))/9000+75);
        mp_bitcnt_t prec = (long int)(digits*BITS_PER_DIGIT+16);
	mpf_set_prec_raw(magifpi, prec);
	mpf_set_prec_raw(mptmp, prec);
	mpf_set_prec_raw(mpt1, prec);
	mpf_set_prec_raw(mpt2, prec);

        usw_ = sw2_(n/2);
	mpzscale = 1;
        mpz_set_ui(magisw, usw_);

        for(i = 0; i < 5; i++)
        {
            mpf_set_d(mpt1, 0.25*mpzscale);
	    mpf_sub(mpt1, mpt1, mpt2);
            mpf_abs(mpt1, mpt1);
            mpf_div(magifpi, magifpi0, mpt1);
            mpf_pow_ui(mptmp, mpten, digits >> 1);
            mpf_mul(magifpi, magifpi, mptmp);
	    mpz_set_f(magipi, magifpi);
            mpz_add(magipi,magipi,magisw);
            mpz_add(product,product,magipi);

	    mpz_import(bns0, b, -1, p, -1, 0, (void*)(hash));
            mpz_add(bns1, bns1, bns0);
            mpz_mul(product,product,bns1);
            mpz_cdiv_q (product, product, bns0);

            bytes = mpz_sizeinbase(product, 256);
            mpzscale=bytes;
            mpz_export(bdata, NULL, -1, 1, 0, 0, product);

            sph_sha256 (&ctx_final_sha256, bdata, bytes);
            sph_sha256_close(&ctx_final_sha256, (void*)(hash));
	}

	const unsigned char *hash_ = (const unsigned char *)hash;
	const unsigned char *target_ = (const unsigned char *)ptarget;
	for (i = 31; i >= 0; i--) {
	      if (hash_[i] != target_[i]) {
		rc = hash_[i] < target_[i];
		break;
	      }
	}
        if (unlikely(rc)) {
            if (opt_debug) {
                bin2hex(hash_str, (unsigned char *)hash, 32);
                bin2hex(target_str, (unsigned char *)ptarget, 32);
                bin2hex(data_str, (unsigned char *)data, 80);
                applog(LOG_DEBUG, "DEBUG: [%d thread] Found share!\ndata   %s\nhash   %s\ntarget %s", thr_id,
                    data_str,
                    hash_str,
                    target_str);
            }
            pdata[19] = data[19];
            goto out;
	  }
    } while (n < max_nonce && !work_restart[thr_id].restart);
     pdata[19] = n;
out:
	mpf_set_prec_raw(magifpi, prec0);
	mpf_set_prec_raw(magifpi0, prec0);
	mpf_set_prec_raw(mptmp, prec0);
	mpf_set_prec_raw(mpt1, prec0);
	mpf_set_prec_raw(mpt2, prec0);
	mpf_clear(magifpi);
	mpf_clear(magifpi0);
	mpf_clear(mpten);
	mpf_clear(mptmp);
	mpf_clear(mpt1);
	mpf_clear(mpt2);
	mpz_clears(magipi, magisw, product, bns0, bns1, NULL);

    *hashes_done = n - first_nonce + 1;
    return rc;
}
