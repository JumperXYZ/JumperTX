/*
 * Copyright (C) JumperTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _FLYSKY_IBUS_H
#define _FLYSKY_IBUS_H

void processFlySkyTelemetryData(uint8_t data, uint8_t* rxBuffer, uint8_t* rxBufferCount);
void flySkySetDefault(int index, uint16_t id, uint8_t subId, uint8_t instance);

// Used by multi protocol
void processFlySkyPacket(const uint8_t *packet);


#endif
