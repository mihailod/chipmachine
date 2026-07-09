#define TRUE            1
#define PHALSE          0
#define STD_ERROR       -1

#define NIB_HI(x)      ((x & 0xF0)>>4)
#define NIB_LO(x)      (x & 0x0F)

#define BYTE_HI(x)      ((x & 0xFF00)>>8)
#define BYTE_LO(x)      (x & 0xFF)

int     TempX, TempY;


#define INC_LIM(x, y)  {TempX=(int)x; if((++TempX)>(int)y) x=y; else x=TempX;}
#define DEC_LIM(x, y)  {TempX=(int)x; if((--TempX)<(int)y) x=y; else x=TempX;}


