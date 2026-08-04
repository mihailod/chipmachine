/*
  mdxtrace -- development-only instrumentation hooks for mdxmini.

  Included by the mdxmini sources ONLY when MDX_TRACE is defined, which is
  done exclusively by tools/mdxtrace/build.sh. Neither the plus nor the mas
  build ever defines it, so every call site below compiles to nothing and
  both shipped binaries are byte-identical to what they were before this
  header existed (see README.md -- object hashes are checked).

  Implementations live in mdxtrace.c.
*/

#ifndef MDXTRACE_HOOK_H
#define MDXTRACE_HOOK_H

/* Sequencer tick boundary. Emitted before the frame's events. */
void mdxtrace_frame(int tempo);

/* Every accepted YM2151 register write, at mdx2151.c's reg_write() choke point. */
void mdxtrace_opm(int adr, int val);

/* PCM8 / ADPCM state changes. `data`/`nbytes` identify the sample by content
   hash rather than by pointer -- pointers are ASLR-dependent and would make
   traces differ between runs of the same binary. */
void mdxtrace_pcm_on(int ch, const void *data, int nbytes);
void mdxtrace_pcm_tie(int ch);
void mdxtrace_pcm_off(int ch);
void mdxtrace_pcm_freq(int ch, int hz);
void mdxtrace_pcm_vol(int ch, int val);
void mdxtrace_pcm_mvol(int val);
void mdxtrace_pcm_pan(int val);

#endif /* MDXTRACE_HOOK_H */
