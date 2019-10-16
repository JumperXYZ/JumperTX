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

#include "opentx.h"

void intmoduleStop(void);
void extmoduleStop(void);

void intmoduleNoneStart(void);
void intmoduleSerialStart(uint32_t baudrate, uint32_t period_half_us);

void extmoduleNoneStart(void);
void extmodulePpmStart(void);
void extmodulePxxStart(void);
void extmoduleSerialStart(uint32_t baudrate, uint32_t period_half_us);
void extmoduleCrossfireStart(void);

void init_no_pulses(uint32_t port)
{
  if (port == INTERNAL_MODULE)
    intmoduleNoneStart();
  else
    extmoduleNoneStart();
}

void disable_no_pulses(uint32_t port)
{
  if (port == INTERNAL_MODULE)
    intmoduleStop();
  else
    extmoduleStop();
}

void init_ppm(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmodulePpmStart();
  }
}

void disable_ppm(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

void init_pxx(uint32_t port)
{
  if (port == EXTERNAL_MODULE)
    extmodulePxxStart();
}

void disable_pxx(uint32_t port)
{
  if (port == EXTERNAL_MODULE)
    extmoduleStop();
}

#if defined(DSM2)
void init_serial(uint32_t port, uint32_t baudrate, uint32_t period_half_us)
{
  if (port == INTERNAL_MODULE) {
    intmoduleSerialStart(baudrate, period_half_us);
  }
  if (port == EXTERNAL_MODULE) {
    extmoduleSerialStart(baudrate, period_half_us);
  }
}

void disable_serial(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
    intmoduleStop();
  }
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
#endif

void init_crossfire(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmoduleCrossfireStart();
  }
}

void disable_crossfire(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
