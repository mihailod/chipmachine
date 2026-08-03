#include "zxay_native.h"

namespace musix::zxay {

std::unique_ptr<Source> createNativeSource(Format f,
                                           const std::vector<uint8_t>& data,
                                           int sampleRate)
{
    switch (f) {
    case Format::stc:
        return createStcSource(data, sampleRate);
    case Format::asc:
        return createAscSource(data, sampleRate);
    case Format::stp:
        return createStpSource(data, sampleRate);
    case Format::sqt:
        return createSqtSource(data, sampleRate);
    case Format::psm:
        return createPsmSource(data, sampleRate);
    case Format::gtr:
        return createGtrSource(data, sampleRate);
    case Format::st11:
        return createSt11Source(data, sampleRate);
    case Format::fxm:
    case Format::amad:
        return createFxmSource(f, data, sampleRate);
    case Format::vtx:
    case Format::psg:
        return createDumpSource(f, data, sampleRate);
    default:
        return nullptr;
    }
}

} // namespace musix::zxay
