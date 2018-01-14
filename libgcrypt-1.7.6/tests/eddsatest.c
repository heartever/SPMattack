#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _GCRYPT_IN_LIBGCRYPT
# include "../src/gcrypt-int.h"
# include "../compat/libcompat.h"
#else
# include <gcrypt.h>
#endif
#include "stopwatch.h"
#define PGM "eddsatest"

static void
die (const char *format, ...)
{
  va_list arg_ptr ;

  va_start( arg_ptr, format ) ;
  putchar ('\n');
  fputs ( PGM ": ", stderr);
  vfprintf (stderr, format, arg_ptr );
  va_end(arg_ptr);
  exit (1);
}


static void
show_sexp (const char *prefix, gcry_sexp_t a)
{
  char *buf;
  size_t size;

  fputs (prefix, stderr);
  size = gcry_sexp_sprint (a, GCRYSEXP_FMT_ADVANCED, NULL, 0);
  buf = malloc (size);
  if (!buf)
    die ("out of core\n");

  gcry_sexp_sprint (a, GCRYSEXP_FMT_ADVANCED, buf, size);
  fprintf (stderr, "%.*s", (int)size, buf);
}

gpg_error_t err;
gcry_sexp_t key_spec, key_pair, pub_key, sec_key;
gcry_mpi_t x;
gcry_sexp_t data;
gcry_sexp_t sig = NULL;
int count;
int p_size = 256;
int is_ed25519 = 1;
int verbose = 3;

//int keybit[512] = {0};
//int keybitind = 0;

static void eddsa_init()
{
  printf ("EdDSA Ed25519 ");
  fflush (stdout);
    
  err = gcry_sexp_build (&key_spec, NULL,
                               "(genkey (ecdsa (curve \"Ed25519\")"
                               "(flags eddsa)))");
     
  err = gcry_pk_genkey (&key_pair, key_spec);
  if (err)
    die ("creating %d bit ECC key failed: %s\n",
             p_size, gcry_strerror (err));
  if (verbose > 2)
    show_sexp ("ECC key:\n", key_pair);

  pub_key = gcry_sexp_find_token (key_pair, "public-key", 0);
  if (! pub_key)
    die ("public part missing in key\n");
  sec_key = gcry_sexp_find_token (key_pair, "private-key", 0);
  if (! sec_key)
    die ("private part missing in key\n");
  gcry_sexp_release (key_pair);
  gcry_sexp_release (key_spec);

  x = gcry_mpi_new (p_size);
  gcry_mpi_randomize (x, p_size, GCRY_WEAK_RANDOM);
  
  err = gcry_sexp_build (&data, NULL,
                           "(data (flags eddsa)(hash-algo sha512)"
                           " (value %m))", x); 
  gcry_mpi_release (x);

  if (err)
    die ("converting data failed: %s\n", gcry_strerror (err));
}

static void eddsa_test()
{
  gcry_sexp_release (sig);
 // while(1)
  {
  	err = gcry_pk_sign (&sig, data, sec_key);
 // 	sleep(1);
  }
//  err = gcry_pk_verify (sig, data, pub_key);
}

int main(void)
{
  eddsa_init();
//while(1)
  eddsa_test();
}
