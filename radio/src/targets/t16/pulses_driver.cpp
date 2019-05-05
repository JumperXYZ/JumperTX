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

//In current implementation there is only external module or internal module on same GPIO as external module
//Because of that all internal module specific methods are removed
//External module driver will act as internal module driver if compiled with definition INTERNAL_MULTIMODULE
//target protocol will be selected via variable - externalModulePort

volatile uint32_t externalModulePort = EXTERNAL_MODULE;

void extmoduleStop(void);
void extmoduleNoneStart(void);
void extmodulePpmStart(void);
void extmodulePxxStart(void);
void extmoduleSerialStart(uint32_t baudrate, uint32_t period_half_us);
void extmoduleCrossfireStart(void);

bool isSharedModule(uint32_t port)
{
#if defined (SHARED_MODULE_GPIO)
  externalModulePort = port;
  return true;
#endif
  return false;
}

void init_no_pulses(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
    //intmoduleNoneStart();
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleNoneStart();
  }
}

void disable_no_pulses(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
    //intmoduleStop();
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

void init_ppm(uint32_t port)
{
  if (port == INTERNAL_MODULE) {

  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmodulePpmStart();
  }
}

void disable_ppm(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

void init_pxx(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmodulePxxStart();
  }
}

void disable_pxx(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
    //intmoduleStop();
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

#if defined(DSM2)
void init_serial(uint32_t port, uint32_t baudrate, uint32_t period_half_us)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleSerialStart(baudrate, period_half_us);
  }
}

void disable_serial(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
#endif

void init_crossfire(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleCrossfireStart();
  }
}

void disable_crossfire(uint32_t port)
{
  if (port == INTERNAL_MODULE) {
  }
  if (isSharedModule(port) || port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
