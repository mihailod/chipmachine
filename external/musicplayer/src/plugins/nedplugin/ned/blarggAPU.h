#ifndef BLARGG_APU_H_INCLUDED
#define BLARGG_APU_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void apuSetup(void (*playcallback)());
void apuShutdown();
void apuRender(void *buf, long len);
void apuWriteReg(int reg, int val);
int apuReadReg(int reg);

#ifdef __cplusplus
}
#endif

#endif //!BLARGG_APU_H_INCLUDED
