#
# $Id: Makefile,v 1.1.1.1 2014/10/15 11:55:35 lloydwood Exp $
#

all	: md5 crc32 sha1
	@ echo "All checksums made"

md5	: md5driver.c md5global.h checksums.a
	cc -o md5 md5driver.c checksum.a

crc32	: crc32driver.c checksums.a
	cc -o crc32 crc32driver.c checksum.a

sha1	: sha1driver.c checksums.a
	cc -o sha1 sha1driver.c checksum.a

checksums.a	: crc32.o md5.o sha1.o
	ar -rc checksum.a crc32.o md5.o sha1.o

sha1.o	: sha1.c sha1.h
	cc -c sha1.c

md5.o	: md5.c md5.h md5global.h
	cc -c md5.c

crc32.o	: crc32.c
	cc -c crc32.c

clean:
	rm -f checksum.a *.o md5 crc32 sha1

#
# $Log: Makefile,v $
# Revision 1.1.1.1  2014/10/15 11:55:35  lloydwood
# Creating saratoga-vallona-dev.
#
# This is the Saratoga transfer protocol Vallona implementation,
# under development by Charles Smith.
#
# Revision 1.2  2014/10/04 03:06:02  chas
# Moved repository from ns1.vallona.net to 192.168.0.1
#
# Revision 1.1.1.1  2013/03/09 07:28:18  chas
# Checksum Library and standalone programs
# crc32, md5 and sha1
#
#

