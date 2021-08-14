#define BZFILE FILE
#define BZ_OK   0
#define BZ_STREAM_END	1

static inline BZFILE *BZ2_bzReadOpen ( int *bzerror, FILE *f, int small, int verbosity, void *unused, int nUnused )
{
	*bzerror = BZ_OK;
	return f;
}

static inline BZFILE *BZ2_bzWriteOpen ( int *bzerror, FILE *f, int blockSize100k, int verbosity, int workFactor )
{
	*bzerror = BZ_OK;
	return f;
}

static inline int BZ2_bzRead ( int *bzerror, BZFILE *b, void *buf, int len )
{
	*bzerror = BZ_OK;
	int rlen = fread(buf, 1, len, b);
	if( rlen < len ){
		*bzerror = BZ_STREAM_END;
	}
	return rlen;
}

static inline void BZ2_bzWrite ( int *bzerror, BZFILE *b, void *buf, int len )
{
	*bzerror = BZ_OK;
	fwrite(buf, 1, len, b);
}

static inline void BZ2_bzReadClose ( int *bzerror, BZFILE *b )
{
	*bzerror = BZ_OK;
}

static inline void BZ2_bzWriteClose ( int *bzerror, BZFILE* f, int abandon, unsigned int* nbytes_in, unsigned int* nbytes_out )
{
	*bzerror = BZ_OK;
}

static inline void *mymalloc(size_t size )
{
	void *buf = malloc(size);
	printf("malloc: %p, %lu\n", buf, size );
	return buf;
}


static inline void myfree( void *buf )
{
	printf("free: %p\n", buf );
	free(buf);
}

#define malloc	mymalloc
#define free	myfree

