#ifdef __ARM_NEON__ || __ARM_NEON
#include "blake2b-arm.c"
#else
#include "blake2b-sse2.c"
#endif
