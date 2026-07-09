#ifndef IXS_SCOPE_CAPTURE_H
#define IXS_SCOPE_CAPTURE_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

#define IXS_SCOPE_BUFFER_SIZE 1024
#define IXS_MAX_SCOPE_CHANNELS 64

struct IxsScopeCapture {
    float buffers[IXS_MAX_SCOPE_CHANNELS][IXS_SCOPE_BUFFER_SIZE];
    int write_pos[IXS_MAX_SCOPE_CHANNELS];
    bool enabled;
    int16_t* snapshot_buf;
    uint32_t snapshot_buf_len;
};

static inline IxsScopeCapture* ixs_scope_capture_create(uint32_t sample_buf_len) {
    IxsScopeCapture* sc = (IxsScopeCapture*)calloc(1, sizeof(IxsScopeCapture));
    if (!sc) return nullptr;
    sc->snapshot_buf = (int16_t*)malloc(sample_buf_len * sizeof(int16_t));
    if (!sc->snapshot_buf) {
        free(sc);
        return nullptr;
    }
    sc->snapshot_buf_len = sample_buf_len;
    sc->enabled = true;
    return sc;
}

static inline void ixs_scope_capture_destroy(IxsScopeCapture* sc) {
    if (sc) {
        free(sc->snapshot_buf);
        free(sc);
    }
}

#endif // IXS_SCOPE_CAPTURE_H
