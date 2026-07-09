#ifndef APUWRAP_H_INCLUDED
#define APUWRAP_H_INCLUDED

#ifdef __cplusplus
extern "C"
#else
extern
#endif
void *UpperPrgPage[4];

#define APUWRAP_SAMPLEPTR UpperPrgPage[0]

#endif //!APUWRAP_H_INCLUDED
