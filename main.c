
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"


// COMMAND-LINE FLAGS
//  -b Make backup
//  -h, --help Help
//  -i Input file
//  -o Output file
//  -t Test -> determine current line ending scheme

char*   src;
char*   dst;
char*   tmpName;
int     backup = 0;
int     test = 0;

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////
int main( int argc, char** argv ) {
    if( argc == 1 )
        return showUsage();

    else if( argc == 2 ) {
        if( !strcmp( argv[ 1 ], "-h" ) || !strcmp( argv[ 1 ], "--help" ) )
            return showUsage();
    }

    long srcSize = 0;
    int err = 0;
    int lineType = 0; // 1 == Unix( \n ), 2 == Windows( \r\n )
    int linesOut = 0;
    int lineFeeds = 0;
    int elms = 0;
    FILE* fIn, *fOut;
    char* buf = NULL;
    struct stat att;

    parseArgs( argc, argv );

    if( src == NULL )
        return -1;

    // Determine the size of the source file
    if( ( err = stat( src, &att ) ) == 0 ) {
        srcSize = att.st_size;

    } else {
        // most likely, the file doesn't exist
        printf( "%s\n", strerror( err ) );
        return err;
    }

    // First make backup copy of src
    if( backup ) {
        tmpName = (char*)malloc( strlen( src )+5 );
        strcpy( tmpName, src );
        tmpName = strcat( tmpName, ".bak" );

        printf( "Backing up %s...\nSaving as %s...\n", src, tmpName );

        sprintf( buf, "cp %s %s", src, tmpName );
        if( !system( buf ) )
            puts( "Backup complete!" );
    }

    // Now read the contents of src in to a buffer...
    buf = (char*)malloc( srcSize+1 );

    int nRead = 0;
    fIn = fopen( src, "r" );
    nRead = fread( &buf[ 0 ], 1, srcSize, fIn );
    fclose( fIn );

    if( nRead != srcSize ) {
        printf( "[ERROR] Unknown error occurred reading %s.\nAborting.\n", src );
        return -1;
    }

    buf[ srcSize ] = '\0';
    lineFeeds = getLFCount( buf );
    lineType = getLEType( buf );

    if( test ) {
        if( lineType ) {
            if( lineType == 1 )
                puts( "UnixEOL" );

            else if( lineType == 2 )
                puts( "WindowsEOL" );
            err = 0;

        } else {
            puts( "Unknown error." );
            puts( "Unable to determine EOL scheme! \'src\' is either empty" );
            puts( "or it simply doesn\'t contain any EOL characters." );
            err = -1;
        }
        return err;
    }

    puts( "Starting conversion process..." );

    if( lineType ) {
        if( lineType == 1 )
            puts( "* Unix -> Windows..." );
        else if( lineType == 2 )
            puts( "* Windows -> Unix..." );
    } else {
        puts( "* ERROR Unable to determine line ending scheme!" );
        goto bail;
    }

    char* leUnix = "\n";
    char* leWin = "\r\n";
    char* p = NULL;
    char** lineBuf = splitLines( buf, &elms );
    char* out = NULL;

    out = (dst != NULL) ? dst : src;
    if( lineBuf != NULL ) {

        fOut = fopen( out, "w" );
        if( fOut != NULL && lineFeeds ) {

            while( linesOut < lineFeeds ) {

                p = lineBuf[ linesOut ];

                if( p != NULL && *p != '\0' ) {
                    // Only write if there is actual content to write...
                    fwrite( p, 1, strlen( p ), fOut );
                }
                // If p == NULL, this would indicate an empty line in
                // the source file...

                if( lineType == 1 ) {
                    // Unix to Windows
                    fwrite( leWin, 1, 2, fOut );
                    linesOut++;

                } else if( lineType == 2 ) {
                    // Windows to Unix
                    fwrite( leUnix, 1, 1, fOut );
                    linesOut++;
                }
            }
            fclose( fOut );
        }
    }

    bail:
    if( linesOut == lineFeeds && err == 0 ) {
        puts( "Line endings converted successfully." );

    } else if( err != 0 ){
        puts( "ERROR: An unknown error has occurred." );
        printf( "You can restore %s from\nit\'s backup file %s\n", src, tmpName );
    }
    return err;
}

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////
/**
 * Function to determine what EOL scheme is being used by a text file.
 * @param s Buffer containing the contents of a text file
 * @return Returns 0 on failure, 1 for Unix, 2 for Windows
 */
int getLEType( const char* s ) {

    if( s == NULL || *s == '\0') {
        #ifdef _DEBUG
        printf( "[DEBUG] %s() ERROR: \'s\' is NULL\n", __FUNCTION__ );
        #endif // _DEBUG
        return -1;
    }

    int i = (int)strlen( s )-1;
    int t = 0;
    char chLast;

    for( ; i >= 0; i-- ) {
        chLast = (int)s[ i ];
        if( chLast == 10 ) { // \n
            if( i > 0 ) {
                i--;
                if( (int)s[ i ] == 13 ) // \r
                    t = 2;
                else
                    t = 1;
                break;

            } else {
                t = 1;
                break;
            }
        }
    }
    return t;
}

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////
/**
 * Function to get the number of EOL characters within a text file
 * @param s1 Buffer containing the contents of a text file
 * @return Returns the number of EOLs detected on success, 0 on failure.
 */
int getLFCount( const char* s1 ) {
    int lf = 0;

    if( s1 != NULL && *s1 != '\0' ) {
        int len = strlen( s1 );
        int p = 0;
        for( ; p < len; p++ ) {
            if( s1[ p ] == '\n' )
                lf++;
        }
    }
    return lf;
}

/////////////////////////////////////////////////////////////////////
// Utility function to split a string buffer in to lines. This works
// for both line ending styles. The function strips line endings from
// the source.
/////////////////////////////////////////////////////////////////////
char** splitLines( char* src, int* elements ) {
    char** buffer = NULL;
    int lfType = getLEType( src );
    int iter = 0;

    if( lfType ) {

        int lines = getLFCount( src );

        if( lines ) {
            buffer = (char**)calloc( lines+8, sizeof( char* ) );

            char buf[ 4096 ] = { "\0" };
            int pos = 0;
            int l = strlen( src );
            char ch;

            int items = 0;
            while( iter < l ) {
                ch = src[ iter ];

                if( ch == '\r' || ch == '\n' ) {
                    // Delimiter. NULL terminate buf and add to buffer
                    if( *buf != '\0' ) {
                        buf[ pos ] = '\0';
                        buffer[ items ] = (char*)realloc( buffer[ items ], strlen( buf )+1 );
                        strcpy( buffer[ items ], buf );
                        buf[ 0 ] = 0;
                        pos = 0;
                        items++;
                    }

                } else {
                    buf[ pos ] = ch;
                    pos++;
                }

                if( iter+1 == l && *buf != '\0' ) {
                    buf[ pos ] = '\0';
                    buffer[ items ] = (char*)realloc( buffer[ items ], strlen( buf )+1 );
                    strcpy( buffer[ items ], buf );
                    items++;
                    break;
                }
                iter++;
            }
        }

        if( elements != NULL )
            *elements = lines;
    }
    return buffer;
}

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////
int showUsage() {
    puts( "/////////////////////////////////////////////////////////////////////" );
    puts( "// eolcvt " );
    puts( "// Copyright \u00A9 2018 ShinobiSoft Software" );
    puts( "/////////////////////////////////////////////////////////////////////" );
    puts( "" );
    puts( "eolcvt is a simple EOL conversion utility designed to easily swap" );
    puts( "EOL characters between Unix line endings and Windows line endings." );
    puts( "eolcvt detects the line endings currently in use by an input file" );
    puts( "and will automatically convert to the other format." );
    puts( "" );
    puts( "USAGE:  eolcvt -b -i <somefile.txt> -o <somefile2.txt>" );
    puts( "   OR:  eolcvt -i <somefile.txt>" );
    puts( "   OR:  eolcvt -t <somefile.txt>\n" );
    puts( "    -b\tMake a backup of the input file before conversion" );
    puts( "    -h\tShows this screen. ( also --help )" );
    puts( "    -i\tSpecifies the input file" );
    puts( "    -o\tSpecifies the output file ( Optional )" );
    puts( "    -t\tTest the input file. Prints current EOL scheme used." );
    puts( "      \tThis will print UnixEOL or WindowsEOL. This flag will also" );
    puts( "      \toverride any other flags on the command-line." );
    puts( "" );
    puts( "NOTE:" );
    puts( "Flags CANNOT be combined in to a single flag ( ie: -bio ). They MUST" );
    puts( "be used as specified above." );
    puts( "" );
    return 0;
}

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////
/**
 * Simple function for parsing command-line arguments and setting
 * initial values for some global variables.
 */
void parseArgs( int argc, char** argv ) {
    int iter = 1;
    char* p;

    while( iter < argc ) {

        p = argv[ iter ];
        if( p[ 0 ] == '-' ) {

            switch( p[ 1 ] ) {

                case 'b': {
                    backup = 1;
                    break;
                }

                case 'i': {
                    if( src == NULL )
                        src = argv[ ++iter ];

                    break;
                }

                case 'o': {
                    dst = argv[ ++iter ];
                    break;
                }

                case 't': {
                    test = 1;
                    src = argv[ ++iter ];
                    break;
                }
            }
        }
        iter++;
    }
}

