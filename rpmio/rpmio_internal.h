#ifndef	H_RPMIO_INTERNAL
#define	H_RPMIO_INTERNAL

/** \ingroup rpmio
 * \file rpmio/rpmio_internal.h
 */

#include <rpm/rpmio.h>
#include <rpm/rpmpgp.h>

#ifdef __cplusplus
extern "C" {
#endif

void fdSetBundle(FD_t fd, rpmDigestBundle bundle);
rpmDigestBundle fdGetBundle(FD_t fd);

/** \ingroup rpmio
 * Attach digest to fd.
 */
void fdInitDigest(FD_t fd, pgpHashAlgo hashalgo, int flags);

/** \ingroup rpmio
 * Update digest(s) attached to fd.
 */
void fdUpdateDigests(FD_t fd, const unsigned char * buf, size_t buflen);

/** \ingroup rpmio
 */
void fdFiniDigest(FD_t fd, pgpHashAlgo hashalgo,
		void ** datap,
		size_t * lenp,
		int asAscii);

/**
 * Read an entire file into a buffer.
 * @param fn		file name to read
 * @retval *bp		(malloc'd) buffer address
 * @retval *blenp	(malloc'd) buffer length
 * @return		0 on success
 */
int rpmioSlurp(const char * fn,
                uint8_t ** bp, ssize_t * blenp);

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMIO_INTERNAL */
