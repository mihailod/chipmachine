/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if your system needs leading underscore to function names in dlsym()
   calls */
/* #undef DLSYM_NEEDS_UNDERSCORE */

/* Define if the DEC AudioFile server driver is compiled */
/* #undef DRV_AF */

/* Define if the Amiga AHI driver is compiled */
/* #undef DRV_AHI */

/* Define if you want an .aiff file writer driver */
#define DRV_AIFF 1

/* Define if the AIX audio driver is compiled */
/* #undef DRV_AIX */

/* Define if the Linux ALSA driver is compiled */
/* #undef DRV_ALSA */

/* Define if the OS/2 direct audio (DART) driver is compiled */
/* #undef DRV_DART */

/* Define if the Windows DirectSound driver is compiled */
/* #undef DRV_DS */

/* Define if the Enlightened Sound Daemon driver is compiled */
/* #undef DRV_ESD */

/* Define if the HP-UX audio driver is compiled */
/* #undef DRV_HP */

/* Define this if you want the Carbon Mac Audio driver */
/* #undef DRV_MAC */

/* Define if the Network Audio System driver is compiled */
/* #undef DRV_NAS */

/* Define if the OpenAL driver is compiled */
/* #undef DRV_OPENAL */

/* Define if the OS/2 MCI driver is compiled */
/* #undef DRV_OS2 */

/* Define if the OpenSL ES driver is compiled */
/* #undef DRV_OSLES */

/* Define if the Open Sound System driver is compiled */
/* #undef DRV_OSS */

/* Define this if you want the MacOS X CoreAudio driver */
#define DRV_OSX 1

/* Define if your system supports binary pipes (i.e. Unix) */
#define DRV_PIPE 1

/* Define if the PulseAudio driver is compiled */
/* #undef DRV_PULSEAUDIO */

/* Define if you want a raw pcm data file writer driver */
#define DRV_RAW 1

/* Define if the Linux SAM9407 driver is compiled */
/* #undef DRV_SAM9407 */

/* Define if the SDL audio driver is compiled */
/* #undef DRV_SDL */

/* Define if the SGI audio driver is compiled */
/* #undef DRV_SGI */

/* Define if the OpenBSD sndio driver is compiled */
/* #undef DRV_SNDIO */

/* Define if you want support for output to stdout */
#define DRV_STDOUT 1

/* Define if the Sun audio driver or compatible (NetBSD, OpenBSD) is compiled
   */
/* #undef DRV_SUN */

/* Define if the Linux Ultra driver is compiled */
/* #undef DRV_ULTRA */

/* Define if you want a .wav file writer driver */
#define DRV_WAV 1

/* Define if the Windows MCI driver is compiled */
/* #undef DRV_WIN */

/* Define if the Windows XAudio2 driver is compiled */
/* #undef DRV_XAUDIO2 */

/* Define if using XAudio 2.8 for Windows8 with XAudio2 driver */
/* #undef DRV_XAUDIO28 */

/* Define to 1 if you have the <AF/AFlib.h> header file. */
/* #undef HAVE_AF_AFLIB_H */

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
/* #undef HAVE_ALSA_ASOUNDLIB_H */

/* Define to 1 if you have the <AL/alc.h> header file. */
/* #undef HAVE_AL_ALC_H */

/* Define to 1 if you have the <AL/alext.h> header file. */
/* #undef HAVE_AL_ALEXT_H */

/* Define to 1 if you have the <AL/al.h> header file. */
/* #undef HAVE_AL_AL_H */

/* Define to 1 if you have the <audio/audiolib.h> header file. */
/* #undef HAVE_AUDIO_AUDIOLIB_H */

/* Define to 1 if you have the <devices/ahi.h> header file. */
/* #undef HAVE_DEVICES_AHI_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <dl.h> header file. */
/* #undef HAVE_DL_H */

/* Define to 1 if you have the <dmedia/audio.h> header file. */
/* #undef HAVE_DMEDIA_AUDIO_H */

/* Define to 1 if you have the <dsound.h> header file. */
/* #undef HAVE_DSOUND_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libgus.h> header file. */
/* #undef HAVE_LIBGUS_H */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <OpenAL/alc.h> header file. */
/* #undef HAVE_OPENAL_ALC_H */

/* Define to 1 if you have the <OpenAL/al.h> header file. */
/* #undef HAVE_OPENAL_AL_H */

/* Define to 1 if you have the <os2.h> header file. */
/* #undef HAVE_OS2_H */

/* Define to 1 if you have the `posix_memalign' function. */
#define HAVE_POSIX_MEMALIGN 1

/* Define if your system provides POSIX.4 threads */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the <pulse/simple.h> header file. */
/* #undef HAVE_PULSE_SIMPLE_H */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <sndio.h> header file. */
/* #undef HAVE_SNDIO_H */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <soundcard.h> header file. */
/* #undef HAVE_SOUNDCARD_H */

/* Define to 1 if you have the `srandom' function. */
#define HAVE_SRANDOM 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sun/audioio.h> header file. */
/* #undef HAVE_SUN_AUDIOIO_H */

/* Define to 1 if you have the <sys/acpa.h> header file. */
/* #undef HAVE_SYS_ACPA_H */

/* Define to 1 if you have the <sys/audioio.h> header file. */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define to 1 if you have the <sys/audio.h> header file. */
/* #undef HAVE_SYS_AUDIO_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/sam9407.h> header file. */
/* #undef HAVE_SYS_SAM9407_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if you have the <xaudio2.h> header file. */
/* #undef HAVE_XAUDIO2_H */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define if you want a debug version of the library */
/* #undef MIKMOD_DEBUG */

/* Define if you want runtime dynamic linking of ALSA, PulseAudio, or EsounD
   drivers */
#define MIKMOD_DYNAMIC 1

/* Define if you want to use SIMD (AltiVec or SSE2) optimizations (Unstable!)
   */
/* #undef MIKMOD_SIMD */

/* Define to 0 or 1 to override MIKMOD_UNIX in mikmod_internals.h. */
/* #undef MIKMOD_UNIX */

/* disable support for module depackers */
/* #undef NO_DEPACKERS */

/* disable the high quality mixer (build only with the standart mixer) */
/* #undef NO_HQMIXER */

/* Name of package */
#define PACKAGE "libmikmod"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libmikmod"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libmikmod 3.3.13"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libmikmod"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.3.13"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if your system is SunOS 4.x */
/* #undef SUNOS */

/* Version number of package */
#define VERSION "3.3.13"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
