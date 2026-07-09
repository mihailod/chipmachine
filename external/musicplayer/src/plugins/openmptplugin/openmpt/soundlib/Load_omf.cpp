/*
 * Load_omf.cpp
 * ------------
 * Purpose: OMF (Onyx Music File) module loader
 * Notes  : Onyx Music File is the MOD-like format of the 1993 Amiga musicdisk
 *          "Jangle" by Onyx (modland: "Onyx Music File/", 24 tunes by Hydra,
 *          Tirana, Tonza and Viba). No standalone replayer ever existed -- the
 *          format was only playable through the original musicdisk executable.
 *          This loader is based on the byte-level format specification decoded
 *          by Martin Bazley (aka swirlythingy) on 6th December 2009, archived at
 *          ftp.modland.com/pub/documents/format_documentation/Onyx Music File (.omf).txt.
 *
 *          The format is essentially a variable-channel ProTracker module with
 *          three quirks: the sequence table, the patterns and the events within
 *          each pattern are all stored *backwards*; there are three padding
 *          bytes before every pattern and every sample data block; and sample
 *          data is unsigned 8-bit. Note periods and effects are standard
 *          ProTracker, except the pattern break (0xD) command takes a raw
 *          hexadecimal value rather than the usual BCD-encoded decimal.
 * Authors: chipmachine (OpenMPT loader, from Martin Bazley's format spec)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "Loaders.h"
#include "MODTools.h"

OPENMPT_NAMESPACE_BEGIN

// File header (everything up to and including the tune name).
struct OMFFileHeader
{
	char     magic[15];     // "Onyx Music File"
	uint8    terminator;    // 0x1A
	uint8    zero;          // 0
	uint8    one;           // 1
	uint8    sequence[256]; // (2 * numPositions) bytes used, stored backwards; rest zero
	uint8    pad[128];      // 0x80 repeated
	uint8    numChannelsM1; // number of channels - 1 (may be more than 4)
	uint8    numPatternsM1; // number of patterns - 1
	uint8    numPositionsX2; // (number of sequence positions - 1) * 2
	char     songName[31];

	bool IsValid() const noexcept
	{
		if(std::memcmp(magic, "Onyx Music File", 15) != 0 || terminator != 0x1A)
			return false;
		if(numChannelsM1 >= MAX_BASECHANNELS)
			return false;
		// The pad area really is 128 bytes of 0x80 in every known module.
		for(uint8 b : pad)
		{
			if(b != 0x80)
				return false;
		}
		return true;
	}
};

MPT_BINARY_STRUCT(OMFFileHeader, 436)


// Per-instrument header (31 of them, immediately after the file header).
struct OMFSampleHeader
{
	char     name[21];
	uint8    volume;        // 0-64
	int16le  finetune;      // finetune * -487
	uint16le length;        // sample length in bytes
	uint16le repeatLength;  // repeat length in bytes, measured from the end of the sample

	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize(MOD_TYPE_MOD);
		mptSmp.nVolume = 4u * std::min(volume, uint8(64));
		// Stored value is finetune * -487; recover the ProTracker -8..7 finetune.
		int32 ft = mpt::saturate_round<int32>(finetune / -487.0);
		ft = Clamp(ft, -8, 7);
		mptSmp.nFineTune = MOD2XMFineTune(static_cast<uint8>(ft) & 0x0F);
	}
};

MPT_BINARY_STRUCT(OMFSampleHeader, 28)


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderOMF(MemoryFileReader file, const uint64 *pfilesize)
{
	OMFFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return ProbeWantMoreData;
	if(!fileHeader.IsValid())
		return ProbeFailure;

	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool CSoundFile::ReadOMF(FileReader &file, ModLoadingFlags loadFlags)
{
	file.Rewind();
	OMFFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader) || !fileHeader.IsValid())
		return false;

	const CHANNELINDEX numChannels = fileHeader.numChannelsM1 + 1;
	const PATTERNINDEX numPatterns = fileHeader.numPatternsM1 + 1;
	const ORDERINDEX numPositions = static_cast<ORDERINDEX>(fileHeader.numPositionsX2 / 2u) + 1;
	const uint16 seqDivisor = static_cast<uint16>(16u * numChannels);

	// 31 instrument headers.
	OMFSampleHeader sampleHeaders[31];
	if(!file.ReadArray(sampleHeaders))
		return false;
	// Two mystery bytes whose meaning is unknown (differ per song) precede the patterns.
	if(!file.CanRead(2))
		return false;

	if(loadFlags == onlyVerifyHeader)
		return true;

	InitializeGlobals(MOD_TYPE_MOD, numChannels);
	m_nSamples = 31;
	m_nMinPeriod = 113 * 4;
	m_nMaxPeriod = 856 * 4;
	m_nSamplePreAmp = 64;
	m_SongFlags.set(SONG_FASTPORTAS | SONG_IMPORTED | SONG_FORMAT_NO_VOLCOL);
	Order().SetDefaultTempoInt(125);
	Order().SetDefaultSpeed(6);
	SetupMODPanning(true);

	m_songName = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, fileHeader.songName);

	// Sequence table: numPositions little-endian 16-bit entries, stored backwards
	// (the entry for the last position comes first). Each entry is the pattern
	// number multiplied by (16 * numChannels).
	Order().resize(numPositions);
	for(ORDERINDEX pos = 0; pos < numPositions; pos++)
	{
		uint16 entry = (fileHeader.sequence[(numPositions - 1 - pos) * 2u])
		             | (static_cast<uint16>(fileHeader.sequence[(numPositions - 1 - pos) * 2u + 1u]) << 8);
		Order()[pos] = static_cast<PATTERNINDEX>(seqDivisor ? entry / seqDivisor : 0);
	}

	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		sampleHeaders[smp - 1].ConvertToMPT(Samples[smp]);
		m_szNames[smp] = mpt::String::ReadBuf(mpt::String::maybeNullTerminated, sampleHeaders[smp - 1].name);
	}

	file.Skip(2);  // mystery word

	// Patterns. Each is preceded by three bytes (two zero bytes + the channel
	// count) and contains 64 events stored backwards, each event holding four
	// bytes per channel from the first channel to the last.
	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(numPatterns);
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		file.Skip(3);  // two zero bytes + channel count
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
		{
			file.Skip(64 * 4 * numChannels);
			continue;
		}

		// Events are stored backwards: the first event in the file is row 63.
		for(ROWINDEX evt = 0; evt < 64; evt++)
		{
			const ROWINDEX row = 63 - evt;
			auto rowBase = Patterns[pat].GetRow(row);
			for(CHANNELINDEX chn = 0; chn < numChannels; chn++)
			{
				const auto data = file.ReadArray<uint8, 4>();
				const uint16 period = data[0] | (static_cast<uint16>(data[1] & 0x0F) << 8);
				const uint8 effect = data[1] >> 4;
				const uint8 sample = data[2];
				const uint8 effectParam = data[3];

				// Repack into the standard ProTracker pattern entry layout so we
				// can reuse the shared period table and effect translation.
				const std::array<uint8, 4> modData =
				{{
					static_cast<uint8>(((period >> 8) & 0x0F) | (sample & 0x10)),
					static_cast<uint8>(period & 0xFF),
					static_cast<uint8>(((sample & 0x0F) << 4) | effect),
					effectParam,
				}};

				ModCommand &m = rowBase[chn];
				const auto [command, param] = ReadMODPatternEntry(modData, m);
				ConvertModCommand(m, command, param);
				// In OMF the pattern break (0xD) value is already hexadecimal,
				// so undo ConvertModCommand's BCD-to-binary conversion.
				if(command == 0x0D)
					m.param = param;
			}
		}
	}

	// Sample data, in instrument order, skipping instruments of length 0. Each
	// block is preceded by three bytes: one zero byte and the little-endian
	// length of the stored data. Sample data is unsigned 8-bit linear.
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
		{
			if(sampleHeaders[smp - 1].length == 0)
				continue;
			file.Skip(1);  // zero byte
			const uint16 dataLength = file.ReadUint16LE();

			ModSample &mptSmp = Samples[smp];
			mptSmp.nLength = dataLength;
			const uint16 repeatLength = sampleHeaders[smp - 1].repeatLength;
			if(repeatLength > 2 && repeatLength <= dataLength)
			{
				mptSmp.nLoopStart = dataLength - repeatLength;
				mptSmp.nLoopEnd = dataLength;
				mptSmp.uFlags.set(CHN_LOOP);
			}

			SampleIO(
				SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::unsignedPCM)
				.ReadSample(mptSmp, file);
		}
	}

	m_modFormat.madeWithTracker = UL_("Onyx");
	m_modFormat.formatName = UL_("Onyx Music File");
	m_modFormat.type = UL_("omf");
	m_modFormat.charset = mpt::Charset::Amiga_no_C1;

	return true;
}

OPENMPT_NAMESPACE_END
