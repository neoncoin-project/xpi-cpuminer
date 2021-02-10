
noinst_LIBRARIES	= libm7m.a

libm7m_a_SOURCES = haval.c keccak.c \
		ripemd.c sha2.c sha2big.c \
		sph_haval.h sph_keccak.h sph_ripemd.h \
		sph_sha2.h poly1305.h sph_whirlpool.h \
		poly1305.c whirlpool.c blake2b.c blake2b.h

libm7m_a_CFLAGS = -Ofast -march=native -flto
