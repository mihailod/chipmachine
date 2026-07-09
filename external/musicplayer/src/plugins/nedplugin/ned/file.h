#define USE_VIRTUAL     0       // Use virtual files?


long   readfile(const char *filename, long offset, long size, void *buffer);
long   writefile(const char *filename, long offset, long size, void *buffer);
long   AppendFile(const char *filename, long offset, long size, void *buffer);
long   get_filesize(const char *filename);






//_Packed class Stream {
//_Packed Union {
//        void    *DataPtr;
//        char    *FileName;
//        };
//        DWORD   Offset;
//        BYTE    Type;           // 0 for files, 1 for Mem
//Stream(const char *FileName);
//};
//
//Stream::Stream(const char *F_Name)
//        {
//        Type=0;
//        Offset=0;
//        OpenFile(F_Name, 
//        }
