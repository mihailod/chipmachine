extern "C"
{
#include "hvl_replay.h"
}

#include "HivelyPlugin.h"
#include "../../tracker_util.h"
#include <coreutils/utils.h>
#include <coreutils/utf8.h>
#include <algorithm>
#include <set>

namespace musix {

class HivelyPlayer : public ChipPlayer
{
public:
    explicit HivelyPlayer(std::string const& fileName)
        : tune(hvl_LoadTune(fileName.c_str(), 44100, 0), &hvl_FreeTune)
    {
        if (tune == nullptr) {
            throw player_exception();
        }
        std::string msg;
        for (auto i = 1; i < tune->ht_InstrumentNr; i++) {
            auto const* name = tune->ht_Instruments[i].ins_Name;
            msg = msg + utils::utf8_encode(name) + " ";
        }

        setMeta("title", tune->ht_Name, "message", msg, "channels",
                tune->ht_Channels, "length", tune->ht_PlayingTime, "format",
                tune->ht_Version == 0xAA ? "AHX" : "Hively");
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        const int frameSize = ((44100 * 2) / 50);

        auto* ptr = reinterpret_cast<int8_t*>(target);
        int len = 0;
        while (len < noSamples - frameSize) {
            // One decode call is one 50Hz replayer frame, so this is already
            // fine enough granularity to see every step of the track go by.
            captureStep(len / 2);
            hvl_DecodeFrame(tune.get(), ptr, ptr + 2, 4);
            ptr += frameSize * 2;
            len += frameSize;
        }
        return len;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return true; }

    bool hasTrackerRows() const override { return true; }

private:
    // AHX/HVL calls a pattern row a "step". The position list maps each channel
    // to a track number (plus a transpose, which the editor does not show in the
    // pattern grid either), and the steps live in ht_Tracks.
    void captureStep(int frameOffset)
    {
        auto* ht = tune.get();
        if (ht == nullptr) { return; }
        if (ht->ht_PosNr == lastPos && ht->ht_NoteNr == lastNote) { return; }
        lastPos = ht->ht_PosNr;
        lastNote = ht->ht_NoteNr;
        if (ht->ht_PosNr < 0 || ht->ht_NoteNr < 0 || ht->ht_NoteNr >= 64 ||
            ht->ht_NoteNr >= ht->ht_TrackLength || ht->ht_Positions == nullptr) {
            return;
        }

        TrackerRow tr;
        tr.frameOffset = frameOffset;
        tr.pattern = static_cast<int16_t>(ht->ht_PosNr);
        tr.row = static_cast<int16_t>(ht->ht_NoteNr);
        tr.numRows = static_cast<int16_t>(ht->ht_TrackLength);
        tr.channels = static_cast<int8_t>(
            std::min<int>(ht->ht_Channels, kTrackerChannels));

        auto const& pos = ht->ht_Positions[ht->ht_PosNr];
        for (int c = 0; c < tr.channels; c++) {
            auto const& step = ht->ht_Tracks[pos.pos_Track[c]][ht->ht_NoteNr];
            auto& cell = tr.cells[c];
            // AHX note 1 is C-1 (the format has no octave 0).
            if (step.stp_Note > 0) {
                tracker::setNote(cell, step.stp_Note - 1 + 12);
            }
            tracker::setInstrument(cell, step.stp_Instrument);
            tracker::setEffect(cell, step.stp_FX, step.stp_FXParam);
        }
        pushTrackerRow(tr);
    }

    std::shared_ptr<hvl_tune> tune;
    int lastPos = -1;
    int lastNote = -1;
};

// ".thx" is the SAME format as ".ahx", not a relative of it: AHX files carry the
// magic "THX\x01" (Abyss' Highest eXperience grew out of The Hidden Xperience),
// and hvl_LoadTune reads them through the same path. Only the extension was
// missing here, so ".thx" tunes fell through to UADE -- which plays them fine,
// but UADE is GPL and absent from the Mac App Store build, where they were
// dropped from the catalog as unplayable. Verified byte-identical handling: a
// ".thx" renamed to ".ahx" loads here and reports the same title.
static const std::set<std::string> supported_ext = {"ahx", "hvl", "thx"};

HivelyPlugin::HivelyPlugin()
{
    hvl_InitReplayer();
}

bool HivelyPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::path_extension(name)) > 0;
}

std::set<std::string> HivelyPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* HivelyPlugin::fromFile(const std::string& name)
{
    return new HivelyPlayer{name};
};

} // namespace musix
extern "C" void hivelyplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::HivelyPlugin>();
    });
}
