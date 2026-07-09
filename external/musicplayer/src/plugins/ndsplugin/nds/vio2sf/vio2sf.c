/*
 * vio2sf glue for chipmachine's ndsplugin.
 *
 * The PSF container / _lib chaining / zlib section loader below is the
 * original ndsplugin loader: it assembles the NDS ROM image and the DeSmuME
 * save-state blob from the .2sf (and any referenced .2sflib files).
 *
 * The emulation core itself is the maintained kode54 / Christopher Snowhill
 * reentrant vio2sf (desmume/state.c). Instead of poking DeSmuME globals and
 * hand-deserialising the save state (as the old core required), we just feed
 * the assembled ROM + state blobs into state_setrom() / state_loadstate() and
 * pull audio with state_render(). All NDS state now lives in a single
 * NDS_state, owned by this translation unit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#include "zlib/zlib.h"
#include "../tagget.h"
#include "vio2sf.h"

#include "desmume/state.h"

/* ------------------------------------------------------------------ */
/* PSF section loader (assembles ROM + save-state into loaderwork)     */
/* ------------------------------------------------------------------ */

static struct
{
	unsigned char *rom;
	unsigned char *state;
	unsigned romsize;
	unsigned statesize;
} loaderwork = {0, 0, 0, 0};

static void load_term(void)
{
	if (loaderwork.rom)
	{
		free(loaderwork.rom);
		loaderwork.rom = 0;
	}
	loaderwork.romsize = 0;
	if (loaderwork.state)
	{
		free(loaderwork.state);
		loaderwork.state = 0;
	}
	loaderwork.statesize = 0;
}

static int load_map(int issave, unsigned char *udata, unsigned usize)
{
	unsigned char *iptr;
	unsigned isize;
	unsigned char *xptr;
	unsigned xsize = getdwordle(udata + 4);
	unsigned xofs = getdwordle(udata + 0);
	if (issave)
	{
		iptr = loaderwork.state;
		isize = loaderwork.statesize;
		loaderwork.state = 0;
		loaderwork.statesize = 0;
	}
	else
	{
		iptr = loaderwork.rom;
		isize = loaderwork.romsize;
		loaderwork.rom = 0;
		loaderwork.romsize = 0;
	}
	if (!iptr)
	{
		unsigned rsize = xofs + xsize;
		if (!issave)
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		iptr = malloc(rsize + 10);
		if (!iptr)
			return XSF_FALSE;
		memset(iptr, 0, rsize + 10);
		isize = rsize;
	}
	else if (isize < xofs + xsize)
	{
		unsigned rsize = xofs + xsize;
		if (!issave)
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		xptr = realloc(iptr, xofs + rsize + 10);
		if (!xptr)
		{
			free(iptr);
			return XSF_FALSE;
		}
		iptr = xptr;
		isize = rsize;
	}
	memcpy(iptr + xofs, udata + 8, xsize);
	if (issave)
	{
		loaderwork.state = iptr;
		loaderwork.statesize = isize;
	}
	else
	{
		loaderwork.rom = iptr;
		loaderwork.romsize = isize;
	}
	return XSF_TRUE;
}

static int load_mapz(int issave, unsigned char *zdata, unsigned zsize, unsigned zcrc)
{
	int ret;
	int zerr;
	uLongf usize = 8;
	uLongf rsize = usize;
	unsigned char *udata;
	unsigned char *rdata;

	udata = malloc(usize);
	if (!udata)
		return XSF_FALSE;

	while (Z_OK != (zerr = uncompress(udata, &usize, zdata, zsize)))
	{
		if (Z_MEM_ERROR != zerr && Z_BUF_ERROR != zerr)
		{
			free(udata);
			return XSF_FALSE;
		}
		if (usize >= 8)
		{
			usize = getdwordle(udata + 4) + 8;
			if (usize < rsize)
			{
				rsize += rsize;
				usize = rsize;
			}
			else
				rsize = usize;
		}
		else
		{
			rsize += rsize;
			usize = rsize;
		}
		free(udata);
		udata = malloc(usize);
		if (!udata)
			return XSF_FALSE;
	}

	rdata = realloc(udata, usize);
	if (!rdata)
	{
		free(udata);
		return XSF_FALSE;
	}

	if (0)
	{
		unsigned ccrc = crc32(crc32(0L, Z_NULL, 0), rdata, usize);
		if (ccrc != zcrc)
			return XSF_FALSE;
	}

	ret = load_map(issave, rdata, usize);
	free(rdata);
	return ret;
}

static int load_psf_one(unsigned char *pfile, unsigned bytes)
{
	unsigned char *ptr = pfile;
	unsigned int code_size;
	unsigned int resv_size;
	unsigned int code_crc;
	if (bytes < 16 || getdwordle(ptr) != 0x24465350)
		return XSF_FALSE;

	resv_size = getdwordle(ptr + 4);
	code_size = getdwordle(ptr + 8);
	code_crc = getdwordle(ptr + 12);

	if (resv_size)
	{
		unsigned resv_pos = 0;
		ptr = pfile + 16;
		if (16 + resv_size > bytes)
			return XSF_FALSE;
		while (resv_pos + 12 < resv_size)
		{
			unsigned save_size = getdwordle(ptr + resv_pos + 4);
			unsigned save_crc = getdwordle(ptr + resv_pos + 8);
			if (getdwordle(ptr + resv_pos + 0) == 0x45564153)
			{
				if (resv_pos + 12 + save_size > resv_size)
					return XSF_FALSE;
				if (!load_mapz(1, ptr + resv_pos + 12, save_size, save_crc))
					return XSF_FALSE;
			}
			resv_pos += 12 + save_size;
		}
	}

	if (code_size)
	{
		ptr = pfile + 16 + resv_size;
		if (16 + resv_size + code_size > bytes)
			return XSF_FALSE;
		if (!load_mapz(0, ptr, code_size, code_crc))
			return XSF_FALSE;
	}

	return XSF_TRUE;
}

typedef struct
{
	char * basepath;
	const char *tag;
	int taglen;
	int level;
	int found;
} loadlibwork_t;

static int load_psf_and_libs(char * basepath, int level, void *pfile, unsigned bytes);

static int load_psfcb(void *pWork, const char *pNameTop, const char *pNameEnd, const char *pValueTop, const char *pValueEnd)
{
	char * full_libpath;
	loadlibwork_t *pwork = (loadlibwork_t *)pWork;

	int ret = xsf_tagenum_callback_returnvaluecontinue;

	if (pNameEnd - pNameTop == pwork->taglen && !strncasecmp(pNameTop, pwork->tag , pwork->taglen))
	{
		unsigned l = pValueEnd - pValueTop;
		char *lib = malloc(l + 1);
		if (!lib)
		{
			ret = xsf_tagenum_callback_returnvaluebreak;
		}
		else
		{
			void *libbuf;
			unsigned libsize;
			memcpy(lib, pValueTop, l);
			lib[l] = '\0';

			full_libpath = (char *) malloc(1024);
			strcpy(full_libpath, pwork->basepath );
			strcat(full_libpath, lib);

			if (!xsf_get_lib(full_libpath, &libbuf, &libsize))
			{
				ret = xsf_tagenum_callback_returnvaluebreak;
			}
			else
			{
				if (!load_psf_and_libs(pwork->basepath, pwork->level + 1, libbuf, libsize))
					ret = xsf_tagenum_callback_returnvaluebreak;
				else
					pwork->found++;
				free(libbuf);
			}
			free(full_libpath);
			free(lib);
		}
	}
	return ret;
}

static int load_psf_and_libs(char * basepath, int level, void *pfile, unsigned bytes)
{
	loadlibwork_t work;

	work.basepath = basepath;
	work.level = level;
	work.tag = "_lib";
	work.taglen = strlen(work.tag);
	work.found = 0;

	if (level <= 10 && xsf_tagenum(load_psfcb, &work, pfile, bytes) < 0)
		return XSF_FALSE;

	if (!load_psf_one(pfile, bytes))
		return XSF_FALSE;

	{
		int n = 2;
		do
		{
			char tbuf[16];
			sprintf(tbuf, "_lib%d", n++);

			work.tag = tbuf;
			work.taglen = strlen(work.tag);
			work.found = 0;
			if (xsf_tagenum(load_psfcb, &work, pfile, bytes) < 0)
				return XSF_FALSE;
		}
		while (work.found);
	}
	return XSF_TRUE;
}

static int load_psf(char * basepath, void *pfile, unsigned bytes)
{
	load_term();

	return load_psf_and_libs(basepath, 1, pfile, bytes);
}

/* ------------------------------------------------------------------ */
/* Public xsf_* API consumed by NDSPlugin.cpp                          */
/* ------------------------------------------------------------------ */

static NDS_state nds_state;
static int nds_loaded = 0;

const char * findlast( const char * s, const char * scan_chr)
{
	const char * start;

	start = s;
	s += strlen( s ) - 1;

	while (s >= start)
	{
		const char *a = scan_chr;
		while (*a != '\0')
			if (*a++ == *s)
				return s + 1;
		--s;
	}
	return 0;
}

int xsf_start(char *filename)
{
	FILE *file;
	unsigned int bytes;
	char *basepath;
	const char *temp;
	const char *pfile = 0;
	int clockdown;

	file = fopen(filename, "rb");
	if (!file)
		return 0;

	fseek(file, 0, SEEK_END);
	bytes = ftell(file);
	fseek(file, 0, SEEK_SET);

	pfile = (const char*)malloc(bytes);
	if (!pfile)
	{
		fclose(file);
		return XSF_FALSE;
	}
	fread((void*)pfile, bytes, 1, file);
	fclose(file);

	basepath = strdup(filename);
	temp = findlast(filename, "/");
	if (temp)
		basepath[temp - filename] = '\0';
	else
		basepath[0] = '\0';

	if (!load_psf(basepath, (void*)pfile, bytes))
	{
		free(basepath);
		free((void*)pfile);
		return XSF_FALSE;
	}

	if (state_init(&nds_state))
	{
		free(basepath);
		free((void*)pfile);
		load_term();
		return XSF_FALSE;
	}
	nds_loaded = 1;

	/* tag-derived emulation parameters (read before loading the state) */
	clockdown = xsf_tagget_int("_clockdown", pfile, bytes, 0);
	nds_state.initial_frames = xsf_tagget_int("_frames", pfile, bytes, -1);
	nds_state.sync_type = xsf_tagget_int("_vio2sf_sync_type", pfile, bytes, 0);
	nds_state.arm9_clockdown_level =
		xsf_tagget_int("_vio2sf_arm9_clockdown_level", pfile, bytes, clockdown);
	nds_state.arm7_clockdown_level =
		xsf_tagget_int("_vio2sf_arm7_clockdown_level", pfile, bytes, clockdown);

	free(basepath);
	free((void*)pfile);

	if (loaderwork.rom)
		state_setrom(&nds_state, loaderwork.rom, loaderwork.romsize);

	if (loaderwork.state)
		state_loadstate(&nds_state, loaderwork.state, loaderwork.statesize);

	return XSF_TRUE;
}

int xsf_get_lib(char *filename, void **buffer, unsigned int *length)
{
	uint8 *filebuf;
	uint32 size;
	FILE *auxfile;

	auxfile = fopen(filename, "rb");

	/* ANTI WINDOWS HACK - try the lower case filename if it can't be found */
	if(!auxfile) {
		char *p = strrchr(filename, '/');
		if(!p) p = filename;
		while(*p) {
			*p = tolower(*p);
			p++;
		}
		auxfile = fopen(filename, "rb");
	}

	if (!auxfile)
		return 0;

	fseek(auxfile, 0, SEEK_END);
	size = ftell(auxfile);
	fseek(auxfile, 0, SEEK_SET);

	filebuf = malloc(size);
	if (!filebuf)
	{
		fclose(auxfile);
		return 0;
	}

	fread(filebuf, size, 1, auxfile);
	fclose(auxfile);

	*buffer = filebuf;
	*length = (unsigned int)size;

	return 1;
}

int xsf_gen(void *pbuffer, unsigned samples)
{
	if (!nds_loaded)
		return 0;
	/* samples == stereo frame count; state_render writes samples*2 int16s */
	state_render(&nds_state, (s16*)pbuffer, samples);
	return samples << 2;
}

void xsf_term(void)
{
	if (nds_loaded)
	{
		state_deinit(&nds_state);
		nds_loaded = 0;
	}
	load_term();
}
