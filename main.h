#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#define EOL_NIX     "\n"
#define EOL_WIN     "\r\n"

int showUsage       ();
int getLEType       ( const char* s );
int getLFCount      ( const char* s1 );
char** splitLines   ( char* src, int* elements );
void parseArgs      ( int argc, char** argv );

#endif // MAIN_H_INCLUDED
