#include "IXSPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// webixs core (vendored at repo-root webixs/, built with -DLINUX so the
// Windows audio/threading paths compile out and the pull-style render API is
// exposed through the vftable).
#include "PlayerIXS.h"
#include "PlayerCore.h"

namespace musix {

namespace {

constexpr int IXS_SAMPLE_RATE = 44100;

} // namespace

// Ixalance player. webixs renders into an internal stereo-S16 block buffer one
// "genAudio" call at a time (block length varies, ~900-1100 frames); we drain
// that block across getSamples() calls. isSongEnd() returns a loop counter that
// becomes nonzero at the first loop point, which we treat as end-of-song.
class IXSPlayer : public ChipPlayer
{
public:
    IXSPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
        : fileData_(data)
    {
        player_ = IXS::IXS__PlayerIXS__createPlayer_00405d90(IXS_SAMPLE_RATE);
        if (player_ == nullptr) {
            throw player_exception("IXS: could not create player");
        }
        // loadIxsFileData reads from the passed buffer; keep fileData_ alive for
        // the player's lifetime. The two FileSFXI* params are for optional
        // external instrument files (unused; always null in the original too).
        char res = (*player_->vftable->loadIxsFileData)(
            player_, fileData_.data(), static_cast<uint32_t>(fileData_.size()),
            nullptr, nullptr, nullptr);
        if (res != 0) {
            (*player_->vftable->delete0)(player_);
            player_ = nullptr;
            throw player_exception("IXS: failed to load file data");
        }
        (*player_->vftable->initAudioOut)(player_);

        std::string title;
        if (const char* t = (*player_->vftable->getSongTitle)(player_)) {
            title = t;
            // The stored title is space/null padded; trim trailing blanks.
            size_t end = title.find_last_not_of(" \t\r\n");
            title = (end == std::string::npos) ? std::string()
                                               : title.substr(0, end + 1);
        }
        if (title.empty()) {
            title = utils::path_basename(fileName);
        }
        setMeta("title", title, "songs", 1, "startSong", 0, "format",
                "Ixalance", "channels", 2);
    }

    ~IXSPlayer() override
    {
        if (player_ != nullptr) {
            (*player_->vftable->delete0)(player_);
        }
    }

    int getHZ() override { return IXS_SAMPLE_RATE; }

    // 'size' is the number of int16 samples (frames * 2) the host can take.
    int getSamples(int16_t* target, int size) override
    {
        int produced = 0;
        while (produced < size) {
            if (blockPos_ >= blockLen_) {
                // Current block drained -- check for end, then render the next.
                if ((*player_->vftable->isSongEnd)(player_)) {
                    break;
                }
                (*player_->vftable->genAudio)(player_);
                block_ = reinterpret_cast<int16_t*>(
                    (*player_->vftable->getAudioBuffer)(player_));
                // getAudioBufferLen reports frames; the buffer is stereo S16.
                blockLen_ = static_cast<int>(
                                (*player_->vftable->getAudioBufferLen)(player_)) *
                            2;
                blockPos_ = 0;
                if (block_ == nullptr || blockLen_ <= 0) {
                    break;
                }
            }
            int n = blockLen_ - blockPos_;
            if (n > size - produced) {
                n = size - produced;
            }
            memcpy(target + produced, block_ + blockPos_,
                   static_cast<size_t>(n) * sizeof(int16_t));
            blockPos_ += n;
            produced += n;
        }
        return produced;
    }

    bool seekTo(int song, int /*seconds*/) override { return song == 0; }

private:
    std::vector<uint8_t> fileData_;
    IXS::PlayerIXS* player_ = nullptr;
    int16_t* block_ = nullptr;
    int blockLen_ = 0; // int16 samples available in current block
    int blockPos_ = 0; // int16 samples already consumed from current block
};

bool IXSPlugin::canHandle(const std::string& name)
{
    // "IXS!" magic at offset 0 is unique and reliable.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[4] = {0};
    size_t n = fread(magic, 1, 4, fp);
    fclose(fp);
    return n == 4 && memcmp(magic, "IXS!", 4) == 0;
}

std::set<std::string> IXSPlugin::getSupportedExtensions() const
{
    return {"ixs"};
}

ChipPlayer* IXSPlugin::fromFile(const std::string& fileName)
{
    auto data = utils::File(fileName).readAll();
    if (data.size() < 4 || memcmp(data.data(), "IXS!", 4) != 0) {
        throw player_exception("Not an Ixalance file");
    }
    return new IXSPlayer{data, fileName};
}

} // namespace musix

extern "C" void ixsplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::IXSPlugin>();
    });
}
