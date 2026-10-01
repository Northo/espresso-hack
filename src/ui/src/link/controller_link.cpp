#include "controller_link.h"
#include "transport.h"
#include <string.h>

static Transport *g_transport;

static void onMessage(const uint8_t *data, size_t len)
{
    if (len == sizeof(proto::StateMsg) && data[0] == proto::MSG_STATE) {
        proto::StateMsg msg;
        memcpy(&msg, data, sizeof(msg));
        app_on_controller_state(msg);
    }
}

bool controller_link_begin()
{
    g_transport = createTransport();
    g_transport->onReceive(onMessage);
    return g_transport->begin();
}

void controller_link_poll()
{
    if (g_transport) g_transport->poll();
}

void controller_link_send_command(proto::CommandId cmd, int16_t value)
{
    if (!g_transport) return;
    proto::CommandMsg msg = {proto::MSG_COMMAND, (uint8_t)cmd, value};
    g_transport->send((const uint8_t *)&msg, sizeof(msg));
}
