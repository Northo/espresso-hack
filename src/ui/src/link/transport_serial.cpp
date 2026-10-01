#ifdef LINK_USE_SERIAL
#include <Arduino.h>
#include "transport.h"
#include "protocol.h"

// Frame: 0x7E | len | payload[len] | crc8(len, payload)
// Resyncs on 0x7E. NOTE: in this mode Serial carries the protocol, so don't print debug text on it.

namespace {

constexpr uint8_t kStart = 0x7E;

uint8_t crc8(const uint8_t *d, size_t n, uint8_t crc = 0)
{
    while (n--) {
        crc ^= *d++;
        for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
    }
    return crc;
}

class SerialTransport : public Transport {
public:
    bool begin() override
    {
        Serial.begin(115200);
        return true;
    }

    void poll() override
    {
        while (Serial.available()) {
            uint8_t b = Serial.read();
            switch (state_) {
            case WAIT_START:
                if (b == kStart) state_ = WAIT_LEN;
                break;
            case WAIT_LEN:
                if (b == 0 || b > proto::MAX_PAYLOAD) {
                    state_ = (b == kStart) ? WAIT_LEN : WAIT_START;
                } else {
                    len_ = b;
                    pos_ = 0;
                    state_ = PAYLOAD;
                }
                break;
            case PAYLOAD:
                buf_[pos_++] = b;
                if (pos_ == len_) state_ = CRC;
                break;
            case CRC:
                if (b == crc8(buf_, len_, crc8(&len_, 1)) && rx_) rx_(buf_, len_);
                state_ = WAIT_START;
                break;
            }
        }
    }

    bool send(const uint8_t *data, size_t len) override
    {
        if (len == 0 || len > proto::MAX_PAYLOAD) return false;
        uint8_t l = len;
        uint8_t crc = crc8(data, len, crc8(&l, 1));
        Serial.write(kStart);
        Serial.write(l);
        Serial.write(data, len);
        Serial.write(crc);
        return true;
    }

private:
    enum { WAIT_START, WAIT_LEN, PAYLOAD, CRC } state_ = WAIT_START;
    uint8_t len_ = 0, pos_ = 0;
    uint8_t buf_[proto::MAX_PAYLOAD];
};

} // namespace

Transport *createTransport()
{
    static SerialTransport t;
    return &t;
}
#endif
