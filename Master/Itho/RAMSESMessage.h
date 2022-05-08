/*
 * Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
 */

#ifndef ITHOPACKET_H_
#define ITHOPACKET_H_

#include "bitbuffer.h"

class RAMSESMessage
{
  public:
    bitbuffer_t bits;

    // from message_t
    uint8_t header;
    uint8_t num_device_ids;
    uint8_t device_id[4][3];
    uint16_t command;
    uint8_t payload_length;
    uint8_t payload[256];
    uint8_t unparsed_length;
    uint8_t unparsed[256];
    uint8_t crc;
};


#endif /* ITHOPACKET_H_ */
