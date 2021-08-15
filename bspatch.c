/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
__FBSDID("$FreeBSD: src/usr.bin/bsdiff/bspatch/bspatch.c,v 1.1 2005/08/06 01:59:06 cperciva Exp $");
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>

static off_t offtin(u_char *buf);

#define SPLIT_LEN	4096

/*ここからが実装部分--------------------------------------------------------------------*/
int newfd;
int oldfd;
FILE *patchCont;
FILE *patchDiff;
FILE *patchExtra;

static void *allocBuffer( void )
{
	void *buf;
	if((buf=malloc(SPLIT_LEN))==NULL){
		err(1,NULL);
	}
	return buf;
}

static void freeBuffer( void *buf )
{
	free(buf);
}

static void *openOldFirm( const char *name, size_t *size )
{
	void *buf;
	int fd;
	if(((oldfd=open(name,O_RDONLY,0))<0)||((*size=lseek(oldfd,0,SEEK_END))==-1)||((buf=malloc(*size+1))==NULL)||(lseek(oldfd,0,SEEK_SET)!=0)||(read(oldfd,buf,*size)!=*size)){
		err(1,"%s",name);
	}
	return buf;
}

static void closeOldFirm( void )
{
	close(oldfd);
}

static void *openNewFirm( const char *name )
{
	if(((newfd=open(name,O_CREAT|O_TRUNC|O_WRONLY,0666))<0)) {
		err(1,"%s",name);
	}
	return allocBuffer();
}

static void putNewFirm( u_char *buf, size_t size )
{
	write(newfd,buf,size);
}

static void	closeNewFirm( u_char *buf )
{
	close(newfd);
	freeBuffer(buf);
}

static size_t openPatchFirm( const char *name )
{
	ssize_t bzctrllen, bzdatalen, newsize;
	u_char header[32];
	
	/* 制御データパートオープン */
	if ((patchCont = fopen(name, "r")) == NULL){
		err(1, "fopen(%s)", name );
	}
	/*
	File format:
		0	8	"BSDIFF40"
		8	8	X
		16	8	Y
		24	8	sizeof(newfile)
		32	X	bzip2(control block)
		32+X	Y	bzip2(diff block)
		32+X+Y	???	bzip2(extra block)
	with control block a set of triples (x,y,z) meaning "add x bytes
	from oldfile to x bytes from the diff block; copy y bytes from the
	extra block; seek forwards in oldfile by z bytes".
	*/

	/* Read header */
	if (fread(header, 1, 32, patchCont) < 32) {
		if (feof(patchCont)){
			errx(1, "Corrupt patch1\n");
		}
		err(1, "fread(%s)", name);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0){
		errx(1, "Corrupt patch2\n");
	}

	/* Read lengths from header */
	bzctrllen=offtin(header+8);
	bzdatalen=offtin(header+16);
	newsize = offtin(header+24);
	if( newsize<0 ){
		errx(1,"Corrupt patch3\n");
	}
	printf("newsize %d, bzctrllen %d, bzdatalen %d\n", newsize, bzctrllen, bzdatalen );

	/* 差分データパートオープン */
	if ((patchDiff = fopen(name, "r")) == NULL){
		err(1, "fopen(%s)", name);
	}
	if (fseeko(patchDiff, 32 + bzctrllen, SEEK_SET)){
		err(1, "fseeko(%s, %lld)", name, (long long)(32 + bzctrllen));
	}
	
	/* 一致データパートオープン */
	if ((patchExtra = fopen(name, "r")) == NULL){
		err(1, "fopen(%s)", name);
	}
	if (fseeko(patchExtra, 32 + bzctrllen + bzdatalen, SEEK_SET)){
		err(1, "fseeko(%s, %lld)", name, (long long)(32 + bzctrllen + bzdatalen));
	}

	return newsize;
}

/* 制御データ取得 */
static size_t getPatchFirmCont( u_char *buf, size_t size )
{
	printf("getPatchFirm: %d\n", size );
	return fread(buf, 1, size, patchCont);
}

/* 差分データ取得 */
static size_t getPatchFirmDiff( u_char *buf, size_t size )
{
	printf("getPatchFirm: %d\n", size );
	return fread(buf, 1, size, patchDiff);
}

/* 一致データ取得 */
static size_t getPatchFirmExtra( u_char *buf, size_t size )
{
	printf("getPatchFirm: %d\n", size );
	return fread(buf, 1, size, patchExtra);
}

static void closePatchFirm( void )
{
	fclose(patchCont);
	fclose(patchDiff);
	fclose(patchExtra);
}
/*--------------------------------------------------------------------ここまで*/

static off_t offtin(u_char *buf)
{
	off_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

int main(int argc,char * argv[])
{
	ssize_t oldsize,newsize;
	u_char buf[8];
	u_char *old, *new;
	off_t oldpos,newpos;
	off_t ctrl[3];
	off_t lenread;
	off_t i,j;
	off_t difflen, splitlen, extralen;

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	newsize = openPatchFirm(argv[3]);
	old = openOldFirm(argv[1], &oldsize);
	new = openNewFirm(argv[2]);

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			lenread = getPatchFirmCont(buf, 8);
			if ( lenread < 8 ) {
				errx(1, "Corrupt patch4\n");
			}
			ctrl[i]=offtin(buf);
		};
		difflen = ctrl[0];	/* 差分全体サイズ */
		splitlen = (difflen>SPLIT_LEN)? SPLIT_LEN:difflen;	/* 差分分割サイズ */
printf("1:difflen %u, splitlen %u\n", difflen, splitlen );

		/* Sanity-check */
		if(newpos+ctrl[0]>newsize)
			errx(1,"Corrupt patch5\n");

		/* Read diff string */
		lenread = getPatchFirmDiff(new, splitlen );
		if ( lenread < splitlen ){
			errx(1, "Corrupt patch6\n");
		}

		/* Add old data to diff string (★差分パート分割対応) */
		for(i=j=0;i<ctrl[0];i++,j++){
			if((oldpos+i>=0) && (oldpos+i<oldsize)){
				if( j>=SPLIT_LEN ){
					putNewFirm(new,SPLIT_LEN);			/* ★ 差分パート分割出力 */
					j -= SPLIT_LEN;
					difflen -= SPLIT_LEN;
					splitlen = (difflen>SPLIT_LEN)? SPLIT_LEN:difflen;
					getPatchFirmDiff(new, splitlen );
printf("2:difflen %u, splitlen %u, j %u\n", difflen, splitlen, j );
				}
				new[j]+=old[oldpos+i];
			}
		}

		putNewFirm(new,difflen);	/* ★ 差分パート分割出力 */

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
			errx(1,"Corrupt patch7\n");

		/* Read extra string (★一致パート分割対応) */
		for(extralen = ctrl[1]; extralen; extralen -= splitlen ){
			splitlen = (extralen>SPLIT_LEN)? SPLIT_LEN:extralen;
			lenread = getPatchFirmExtra(new, splitlen );		/* ★ 一致パート分割入力 */
			if ( lenread < splitlen ){
				errx(1, "Corrupt patch8\n");
			}
			putNewFirm(new,splitlen);		/* ★ 一致パート分割出力 */
		}

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	/* close each firm */
	closePatchFirm();
	closeOldFirm();
	closeNewFirm(new);

	return 0;
}
