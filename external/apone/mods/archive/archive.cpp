
#include "archive.h"

#define MINIZ_HEADER_FILE_ONLY
extern "C" {
#include <miniz/miniz.c>
}
//#include "ziplib/zip.h"

#include <vector>
#include <cstring>
#include <coreutils/log.h>
//#define _UNIX
#ifdef _WIN32
#include <windows.h>
#endif
#include  "unrar/dll.hpp"

using namespace std;

namespace utils {

// ZIP central-directory filenames are stored either as UTF-8 (general-purpose
// bit 11 set) or, historically, in the original OEM code page -- overwhelmingly
// CP437 for DOS/PKZIP-era archives. Scene compo zips still ship CP437 names, so
// a member like "R\x9bly - Tiatronic/..." carries a lone 0x9b byte. On macOS
// (APFS) an invalid-UTF-8 path is rejected: mkdir/open fail, miniz's extract
// silently no-ops, the file never lands, and playback later hits ENOENT.
// Translate such names to valid UTF-8 so the destination path is representable
// AND internally consistent (we still locate the member by its raw stored name).
namespace {

// CP437 high half (0x80-0xFF) -> Unicode code point.
static const unsigned short CP437_HIGH[128] = {
    0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,
    0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
    0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,
    0x00FF,0x00D6,0x00DC,0x00A2,0x00A3,0x00A5,0x20A7,0x0192,
    0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,
    0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
    0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
    0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
    0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,
    0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
    0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,
    0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
    0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,
    0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
    0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,
    0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
};

// Reject the byte sequences macOS will refuse in a path component: any
// malformed UTF-8 (lone continuation bytes, truncated multibyte, overlong or
// out-of-range sequences). Pure ASCII and well-formed UTF-8 pass through.
bool isValidUtf8(const string& s) {
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        int extra;
        unsigned int cp;
        if (c < 0x80) { i++; continue; }
        else if ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
        else return false;
        if (i + extra >= n) return false;
        for (int k = 1; k <= extra; k++) {
            unsigned char cc = (unsigned char)s[i + k];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        // Reject overlong encodings and code points beyond U+10FFFF.
        if ((extra == 1 && cp < 0x80) || (extra == 2 && cp < 0x800) ||
            (extra == 3 && cp < 0x10000) || cp > 0x10FFFF)
            return false;
        i += extra + 1;
    }
    return true;
}

void appendUtf8(string& out, unsigned int cp) {
    if (cp < 0x80) out.push_back((char)cp);
    else if (cp < 0x800) {
        out.push_back((char)(0xC0 | (cp >> 6)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    } else {
        out.push_back((char)(0xE0 | (cp >> 12)));
        out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back((char)(0x80 | (cp & 0x3F)));
    }
}

// Return a filesystem-representable UTF-8 name. Already-valid UTF-8 (whether the
// archive set the UTF-8 flag or an authoring tool wrote UTF-8 without it) is kept
// verbatim; otherwise the bytes are decoded as CP437.
string fsSafeName(const string& name) {
    if (isValidUtf8(name)) return name;
    string out;
    out.reserve(name.size());
    for (unsigned char c : name) {
        if (c < 0x80) out.push_back((char)c);
        else appendUtf8(out, CP437_HIGH[c - 0x80]);
    }
    return out;
}

} // namespace

/*
class ExtArchive : public Archive {
	File extract(const string &name) {
		system("lha x " + name);
	}
};*/

class ZipFile : public Archive {
public:
	ZipFile(const string &fileName, const string &workDir = ".") : workDir(workDir) {
		//zipFile = zip_open(fileName.c_str(), 0, NULL);
		memset(&zipArchive, 0, sizeof(zipArchive));
		if(!mz_zip_reader_init_file(&zipArchive, fileName.c_str(), 0))
			throw archive_exception("Could not open zip file");
	}

	~ZipFile() {
		close();
	}

	void close() {
		mz_zip_reader_end(&zipArchive);
	}

	File extract(const string &name) {

		// The member is located inside the archive by its raw stored name (the
		// exact bytes miniz read from the central directory), but written to disk
		// under a filesystem-representable UTF-8 path. CP437 names carrying bytes
		// that aren't valid UTF-8 are otherwise rejected by macOS, so the extract
		// silently fails and the file never appears at getName() -> ENOENT later.
		File file(workDir + "/" + fsSafeName(name));
		// A zip directory entry (name ends with '/') is not a file: extracting it
		// would drop a 0-byte FILE where a folder belongs, so a sibling member
		// like "dir/song.mp3" then fails to write ("Not a directory"). Skip it.
		if (!name.empty() && name.back() == '/')
			return file;
		// Members can be nested in folders (e.g. "Atari 2600 Music Compo/x.mp3").
		// mz_zip_reader_extract_file_to_file does NOT create intermediate dirs, so
		// make the parent first or the extract silently fails and the file never
		// appears at getName().
		auto parent = utils::path_directory(file.getName());
		// A previous (buggy) run that extracted the folder's own dir entry left a
		// 0-byte FILE where this parent directory belongs; drop it so makedirs can
		// create the real dir (otherwise mkdir keeps failing and the member never
		// extracts -> "Not a directory" at play time).
		if (!parent.empty() && utils::File::exists(parent) &&
		    !utils::File(parent).isDir())
			utils::File::remove(parent);
		utils::makedirs(parent);
		// NB: the 3rd arg is the destination FILE path, not a directory -- passing
		// workDir here wrote every member onto the dir itself (a no-op/failure), so
		// the extracted file never appeared at getName(). Use the full file path.
		if (!mz_zip_reader_extract_file_to_file(&zipArchive, name.c_str(),
		                                        file.getName().c_str(), 0))
			LOGW("Failed to extract '%s' from zip to '%s'", name,
			     file.getName());
		return file;

		/*int i = zip_name_locate(zipFile, name.c_str(), ZIP_FL_NOCASE);
		if(i >= 0) {
			struct zip_file *zf = zip_fopen_index(zipFile, i, 0);
			File file(workDir + "/" + name);
			vector<uint8_t> buffer(2048);
			while(true) {
				int bytes = zip_fread(zf, &buffer[0], buffer.size());
				if(bytes > 0)
					file.write(&buffer[0], bytes);
				else
					break;
			}
			file.close();
			zip_fclose(zf);
			return file;
		}
		return File();*/
	}

	virtual string nameFromPosition(int pos) const {
	mz_zip_archive_file_stat file_stat;
    if(!mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(&zipArchive), pos, &file_stat))
    {}
	return string(file_stat.m_filename);

		//struct zip_stat sz;
		//zip_stat_index(zipFile, pos, 0, &sz);
		//return string(sz.name);
	}

	virtual int totalFiles() const {
		return mz_zip_reader_get_num_files(const_cast<mz_zip_archive*>(&zipArchive));
		//return zip_get_num_files(zipFile);
	}

private:
	mz_zip_archive zipArchive;
	//struct zip *zipFile;
	string workDir;
};


class RarFile : public Archive {
public:
	RarFile(const string &fileName, const string &workDir = ".") : workDir(workDir) {
		//fprintf(stderr, "CONSTR");
		//fflush(stderr);
		RAROpenArchiveDataEx archiveInfo;
		memset(&archiveInfo, 0, sizeof(archiveInfo));
		archiveInfo.CmtBuf = NULL;
		archiveInfo.OpenMode = RAR_OM_EXTRACT;
		archiveInfo.ArcName = (char*)fileName.c_str();
		rarFile = RAROpenArchiveEx(&archiveInfo);
		if(archiveInfo.OpenResult != 0) {
			throw archive_exception((std::string("Bad RAR code ") + std::to_string(archiveInfo.OpenResult)).c_str());
		};
		currentPos = 0;
		RHCode = RARReadHeaderEx(rarFile, &fileInfo);


	}

	~RarFile() {
		//fprintf(stderr, "DESTR");
		//fflush(stderr);
		RARCloseArchive(rarFile);
	}

	File extract(const string &name) {
		//RARHeaderDataEx fileInfo;
		//int RHCode = RARReadHeaderEx(rarFile, &fileInfo);

		//int RHCode = RARReadHeaderEx(rarFile, &fileInfo);
		//LOGD("RHCode %d %s", RHCode, fileInfo.FileName);
		//if(RHCode !=0)
		//	return File();

		int PFCode = RARProcessFile(rarFile, RAR_EXTRACT, (char*)workDir.c_str(), NULL);

		//LOGD("extract %d", PFCode);

		RHCode = RARReadHeaderEx(rarFile, &fileInfo);

		currentPos++;

		File f { workDir + "/" + fileInfo.FileName };

		return f;
	}

	virtual string nameFromPosition(int pos) const {

		//LOGD("POS %d vs %d", pos , currentPos);
		while(currentPos < pos) {
			int PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL);
			//LOGD("PFCode %d", PFCode);

			RHCode = RARReadHeaderEx(rarFile, &fileInfo);

			currentPos++;
		}

		if(RHCode != 0)
			return "";

		//int RHCode = RARReadHeaderEx(rarFile, &fileInfo);
		//LOGD("pos %d %s", currentPos, fileInfo.FileName);
		//if(RHCode !=0)
		//	return "";
		return fileInfo.FileName;
	}

	virtual int totalFiles() const {
		return -1;
	}

private:

	HANDLE rarFile;
	mutable int currentPos;
	//struct zip *zipFile;
	mutable RARHeaderDataEx fileInfo;
	mutable int RHCode;
	string workDir;
};


Archive *Archive::open(const std::string &fileName, const std::string &targetDir, int type) {
	// Match the extension case-insensitively: web caches name files after the
	// source URL, so an uppercase ".ZIP" (e.g. modland/Fujiology "SMELLS.ZIP")
	// must still open. A case-sensitive endsWith(".zip") returned nullptr here,
	// and callers that detected the archive by magic then dereferenced it.
	auto lower = utils::toLower(fileName);
	if(type == TYPE_ZIP || utils::endsWith(lower, ".zip"))
		return new ZipFile(fileName, targetDir);
	else if(type == TYPE_RAR || utils::endsWith(lower, ".rar"))
		return new RarFile(fileName, targetDir);
	return nullptr;
}

bool Archive::canHandle(const std::string &name) {
	return utils::endsWith(utils::toLower(name), ".zip");
}

} // namespace utils
