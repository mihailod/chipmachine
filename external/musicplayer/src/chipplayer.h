#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace musix {

using MetaVar = std::variant<std::string, double, uint32_t>;

// ---------------------------------------------------------------------------
// Tracker pattern feed (optional -- only the tracker-style engines have one)
//
// A player that KNOWS which pattern row it is playing (libopenmpt, libxmp, the
// AHX/HVL replayer) pushes one TrackerRow per row transition from inside
// getSamples(). The host drains the queue right afterwards on the same (decode)
// thread and stamps each row with the absolute sample position it starts at, so
// the on-screen pattern can be synced to what the speakers are playing rather
// than to what the decoder has already rendered -- the audio FIFO runs up to
// ~1.5s ahead, which would otherwise show notes long before you hear them.
//
// Only the first kTrackerChannels channels are captured; the display shows four
// columns and anything beyond that would be thrown away.
//
// The cells carry already-FORMATTED text, not raw numbers: each engine spells
// notes and effects in its own convention (libopenmpt has a formatter that
// knows the quirks of every format it loads) and the display has no business
// re-deriving that.
// ---------------------------------------------------------------------------
inline constexpr int kTrackerChannels = 4;

struct TrackerCell
{
    char note[4]{}; // "C-4"; "===" note off, "^^^" cut; empty = no note
    char inst[3]{}; // "01"; empty = none
    char fx[4]{};   // "A0F"; empty = none
};

struct TrackerRow
{
    int frameOffset = 0; // frames from the start of the getSamples() call
    int16_t pattern = 0;
    int16_t row = 0;
    int16_t numRows = 0;
    int8_t channels = 0;
    TrackerCell cells[kTrackerChannels];
};

class player_exception : public std::exception
{
public:
    explicit player_exception(const std::string& msg = "") : msg(msg) {}
    const char* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

class ChipPlayer
{
public:
    using Callback =
        std::function<void(const std::vector<std::string>& meta)>;

    virtual ~ChipPlayer() = default;
    virtual int getSamples(int16_t* target, int size) = 0;

    virtual int getHZ() { return 44100; }

    // For the streaming (fromStream) path only: signals that no more input bytes
    // will arrive on the fifo, so the player can flush and end cleanly. No-op for
    // ordinary file players.
    virtual void endStream() {}

    virtual bool setParameter(const std::string& /*name*/, int32_t /*value*/)
    {
        return false;
    }
    virtual bool setParameter(const std::string& /*name*/,
                              const std::string& /*value*/)
    {
        return false;
    }

    MetaVar const& meta(std::string const& what) { return metaData[what]; }

    void setMeta()
    {
        if (!changedMeta.empty()) {
            //printf("ChipPlayer: setMeta - Processing %zu changed metadata items.\n", changedMeta.size());
            for (const auto& cb : callbacks) {
                cb(changedMeta);
            }
            //printf("ChipPlayer: setMeta - Calling callbacks.\n");
            for(auto&& cm : changedMeta) {
                //printf("ChipPlayer: setMeta - Adding '%s' to lastMeta.\n", cm.c_str());
                lastMeta.insert(cm);
            }
            changedMeta.clear();
            //printf("ChipPlayer: setMeta - finished processing.\n");
        }
    }

    std::optional<std::string> getChangedMeta()
    {
        if (lastMeta.empty()) {
            //printf("ChipPlayer: getChangedMeta - lastMeta is empty.\n");
            return std::nullopt;
        }
        auto res = *lastMeta.begin();
        //printf("ChipPlayer: getChangedMeta - Returning '%s'\n", res.c_str());
        lastMeta.erase(res);
        return res;
    }

    template <typename T, typename... A,
              typename = typename std::enable_if<std::is_integral_v<T>>::type>
    void setMeta(const std::string& what, T value, const A&... args)
    {
        metaData[what] = static_cast<uint32_t>(value);
        changedMeta.push_back(what);
        setMeta(args...);
    }

    template <typename... A>
    void setMeta(const std::string& what, double value, const A&... args)
    {
        metaData[what] = value;
        changedMeta.push_back(what);
        setMeta(args...);
    }

    template <typename... A>
    void setMeta(const std::string& what, const MetaVar& value,
                 const A&... args)
    {
        metaData[what] = value;
        changedMeta.push_back(what);
        setMeta(args...);
    }

    template <typename... A>
    void setMeta(const std::string& what, const std::string& value,
                 const A&... args)
    {
        metaData[what] = value;
        changedMeta.push_back(what);
        setMeta(args...);
    }

    template <typename... A>
    void setMeta(const std::string& what, const char* value, const A&... args)
    {
        metaData[what] = std::string(value);
        changedMeta.push_back(what);
        setMeta(args...);
    }

    virtual bool seekTo(int  /*song*/, int  /*seconds*/ = -1) { return false; }

    // True if this player feeds the tracker pattern view (see TrackerRow).
    virtual bool hasTrackerRows() const { return false; }

    // Hand over everything pushed during the last getSamples(). Must be called
    // on the decode thread, immediately after getSamples(), so the frameOffsets
    // still refer to that call. Keeps the vector's capacity for reuse.
    void takeTrackerRows(std::vector<TrackerRow>& out)
    {
        out.swap(trackerRows);
        trackerRows.clear();
    }

    void onMeta(const Callback& callback)
    {
        callbacks.push_back(callback);
        std::vector<std::string> meta;
        meta.reserve(metaData.size());
        for (auto& md : metaData) {
            meta.push_back(md.first);
        }
        callback(meta);
    }

protected:
    // Bounded: if nobody drains (the console/cm builds never do) rows are simply
    // dropped rather than growing without limit.
    void pushTrackerRow(const TrackerRow& r)
    {
        if (trackerRows.size() < 512) { trackerRows.push_back(r); }
    }

    std::vector<TrackerRow> trackerRows;

    std::unordered_map<std::string, MetaVar> metaData;
    std::vector<Callback> callbacks;
    std::vector<std::string> changedMeta;
    std::unordered_set<std::string> lastMeta;
};

} // namespace musix
