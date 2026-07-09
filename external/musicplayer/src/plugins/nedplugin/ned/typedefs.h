#ifndef _TYPEDEFS_H_INCLUDED
        #define _TYPEDEFS_H_INCLUDED

        #ifndef NULL
                #define NULL    0
        #endif

        typedef unsigned char   uBYTE;
        typedef   signed char   sBYTE;    
        typedef uBYTE            BYTE;
        typedef unsigned short  uWORD;   
        typedef   signed short  sWORD;   
        typedef uWORD            WORD;
        /* DWORD must be exactly 32 bits to match the on-disk .ned layout the
         * MS-DOS/Win32 tracker wrote ("long" is 64-bit on LP64 macOS/Linux). */
        typedef unsigned int    uDWORD;
        typedef signed int      sDWORD;
        typedef uDWORD           DWORD;

#endif  // _TYPEDEFS_H_INCLUDED
