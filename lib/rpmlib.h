#ifndef H_RPMLIB
#define H_RPMLIB

/* This is the *only* module users of rpmlib should need to include */

#include <db.h>

/* it shouldn't need these :-( */
#include "dbindex.h"
#include "falloc.h"
#include "header.h"

/* these tags are for both the database and packages */
/* none of these can be 0 !!                         */

#define RPMTAG_NAME  			1000
#define RPMTAG_VERSION			1001
#define RPMTAG_RELEASE			1002
#define RPMTAG_SERIAL   		1003
#define	RPMTAG_SUMMARY			1004
#define RPMTAG_DESCRIPTION		1005
#define RPMTAG_BUILDTIME		1006
#define RPMTAG_BUILDHOST		1007
#define RPMTAG_INSTALLTIME		1008
#define RPMTAG_SIZE			1009
#define RPMTAG_DISTRIBUTION		1010
#define RPMTAG_VENDOR			1011
#define RPMTAG_GIF			1012
#define RPMTAG_XPM			1013
#define RPMTAG_COPYRIGHT                1014
#define RPMTAG_PACKAGER                 1015
#define RPMTAG_GROUP                    1016
#define RPMTAG_CHANGELOG                1017
#define RPMTAG_SOURCE                   1018
#define RPMTAG_PATCH                    1019
#define RPMTAG_URL                      1020
#define RPMTAG_OS                       1021
#define RPMTAG_ARCH                     1022
#define RPMTAG_PREIN                    1023
#define RPMTAG_POSTIN                   1024
#define RPMTAG_PREUN                    1025
#define RPMTAG_POSTUN                   1026
#define RPMTAG_FILENAMES		1027
#define RPMTAG_FILESIZES		1028
#define RPMTAG_FILESTATES		1029
#define RPMTAG_FILEMODES		1030
#define RPMTAG_FILEUIDS			1031
#define RPMTAG_FILEGIDS			1032
#define RPMTAG_FILERDEVS		1033
#define RPMTAG_FILEMTIMES		1034
#define RPMTAG_FILEMD5S			1035
#define RPMTAG_FILELINKTOS		1036
#define RPMTAG_FILEFLAGS		1037
#define RPMTAG_ROOT                     1038

#define RPMFILE_STATE_NORMAL 		0
#define RPMFILE_STATE_REPLACED 		1

/* these can be ORed together */
#define RPMFILE_CONFIG			1
#define RPMFILE_DOC			2

struct rpmdb {
    faFile pkgs;
    dbIndex * nameIndex, * fileIndex, * groupIndex;
};

int rpmdbOpen (char * prefix, struct rpmdb *rpmdb, int mode, int perms);
    /* 0 on error */
int rpmdbCreate (struct rpmdb *rpmdb, int mode, int perms);
    /* this fails if any part of the db already exists */
void rpmdbClose (struct rpmdb *rpmdb);

unsigned int rpmdbFirstRecNum(struct rpmdb * rpmdb);
unsigned int rpmdbNextRecNum(struct rpmdb * rpmdb, unsigned int lastOffset);  
    /* 0 at end */

Header rpmdbGetRecord(struct rpmdb * rpmdb, unsigned int offset);
int rpmdbFindByFile(struct rpmdb * rpmdb, char * filespec, 
		    dbIndexSet * matches);

#endif
