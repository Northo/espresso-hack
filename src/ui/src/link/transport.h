#pragma once
#include <stddef.h>
#include <stdint.h>

// Byte-message transport. Messages are delivered whole (one payload per call).
class Transport {
public:
    virtual ~Transport() {}
    virtual bool begin() = 0;
    // Call from loop(); invokes the receive callback from the caller's context.
    virtual void poll() = 0;
    virtual bool send(const uint8_t *data, size_t len) = 0;
    void onReceive(void (*cb)(const uint8_t *data, size_t len)) { rx_ = cb; }

protected:
    void (*rx_)(const uint8_t *, size_t) = nullptr;
};

// Selected at build time: serial if LINK_USE_SERIAL is defined, otherwise ESP-NOW.
Transport *createTransport();
