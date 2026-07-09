#include <stdio.h>

#include "file.h"

long   readfile(const char *filename, long offset, long size, void *buffer)
  {
  unsigned long FilePos;
  FILE *fp;
  if(USE_VIRTUAL) return(0);
  else {
    fp=fopen(filename, "rb");
    if(!fp) return 0;
    fseek(fp, offset, SEEK_SET);
    fread(buffer, 1, size, fp);
    FilePos=ftell(fp);
    fclose(fp);
    return(FilePos);
    }
  }

long   writefile(const char *filename, long offset, long size, void *buffer)
  {
  unsigned long FilePos;
  FILE *fp;
  if(USE_VIRTUAL) return(0);
  else {
    fp=fopen(filename, "wb+");
//    fseek(fp, offset, SEEK_SET);
    fwrite(buffer, 1, size, fp);
    FilePos=ftell(fp);
    fclose(fp);
    return(FilePos);
    }
  }


long   AppendFile(const char *filename, long offset, long size, void *buffer)
  {
  unsigned long FilePos;
  FILE *fp;
  if(USE_VIRTUAL) return(0);
  else {
    fp=fopen(filename, "ab+");
    fseek(fp, offset, SEEK_SET);
    fwrite(buffer, 1, size, fp);
    FilePos=ftell(fp);
    fclose(fp);
    return(FilePos);
    }
  }


long   get_filesize(const char *filename)
  {
  FILE  *fp;
  long  size;
  if(USE_VIRTUAL) return(0);
  else {
    fp=fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    size=ftell(fp);
    fclose(fp);
    return(size);
    }
  }
    

