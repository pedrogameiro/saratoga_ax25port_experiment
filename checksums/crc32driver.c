/*----------------------------------------------------------------------------*\
 *  NAME:
 *     main() - main function for CRC-32 generation
 *  DESCRIPTION:
 *     Computes the CRC-32 value for the set of files named in the command-
 *     line arguments.
 *  ARGUMENTS:
 *     argc - command-line-argument count
 *     argv - command-line-argument strings
 *  RETURNS:
 *     err - 0 on success or executes exit(1) on error
 *  ERRORS:
 *     - file errors
\*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*\
 *  Local functions
\*----------------------------------------------------------------------------*/

extern int Crc32_ComputeFile( FILE *file, unsigned long *outCrc32 );

/*
 * ----------------------------------------------------------------------------
 */
#ifdef CRC32DRIVER
int main( int argc, const char *argv[] )
{
    FILE *file = NULL;
    const char *filename;
    unsigned long argIdx;
    unsigned long crc32;
    int err;

    /** compute crcs **/
    if (argc < 2) {
        /** read from 'stdin' if no arguments given **/
        err = Crc32_ComputeFile( stdin, &crc32 );
        if (err == -1) goto ERR_EXIT;
        printf("crc32 = 0x%08lX for (stdin)\n", crc32 );
    } else {
        /** report named files in sequence **/
        for (argIdx=1; argIdx < (unsigned long) argc; argIdx++) {
            filename = argv[argIdx];
            file = fopen( filename, "rb" );
            if (file == NULL) {
                fprintf( stderr, "error opening file \"%s\"!\n", filename );
                goto ERR_EXIT;
            }
            err = Crc32_ComputeFile( file, &crc32 );
            if (err == -1) goto ERR_EXIT;
            printf("CRC32 (%s) = 0x%08lX\n", filename, crc32);
            err = fclose( file );
            file = NULL;
            if (err == EOF) {
                fprintf( stderr, "error closing file \"%s\"!\n", filename );
                goto ERR_EXIT;
            }
        }
    }
    return( 0 );

    /** error exit **/
ERR_EXIT:
    if (file != NULL) fclose( file );
    exit( 1 );
}

/*
 * $Log: crc32driver.c,v $
 * Revision 1.1.1.1  2014/10/15 11:55:35  lloydwood
 * Creating saratoga-vallona-dev.
 *
 * This is the Saratoga transfer protocol Vallona implementation,
 * under development by Charles Smith.
 *
 * Revision 1.1.1.1  2013/03/09 07:28:18  chas
 * Checksum Library and standalone programs
 * crc32, md5 and sha1
 *
 */
#endif
