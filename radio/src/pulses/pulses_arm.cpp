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

uint8_t s_pulses_paused = 0;
uint8_t s_current_protocol[NUM_MODULES] = { MODULES_INIT(255) };
uint16_t failsafeCounter[NUM_MODULES] = { MODULES_INIT(100) };
uint8_t moduleFlag[NUM_MODULES] = { 0 };

ModulePulsesData modulePulsesData[NUM_MODULES] __DMA;
TrainerPulsesData trainerPulsesData __DMA;

#if defined(CROSSFIRE)
uint8_t createCrossfireChannelsFrame(uint8_t * frame, int16_t * pulses);
#endif

uint8_t getRequiredProtocol(uint8_t port)
{
  uint8_t required_protocol;

  switch (port) {
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBT16) || defined(PCBT16HD)
    case INTERNAL_MODULE:
      switch (g_model.moduleData[INTERNAL_MODULE].type) {
#if defined(TARANIS_INTERNAL_PPM)
        case MODULE_TYPE_PPM:
          required_protocol = PROTO_PPM;
          break;
#endif
#if defined(INTERNAL_MULTIMODULE)
        case MODULE_TYPE_MULTIMODULE:
          required_protocol = PROTO_MULTIMODULE;
          break;
#endif
        case MODULE_TYPE_XJT:
          required_protocol = PROTO_PXX;
          break;
        default:
          required_protocol = PROTO_NONE;
          break;
      }
      break;
#endif

    default:
      port = EXTERNAL_MODULE; // ensure it's external module only
      switch (g_model.moduleData[EXTERNAL_MODULE].type) {
        case MODULE_TYPE_PPM:
          required_protocol = PROTO_PPM;
          break;
        case MODULE_TYPE_XJT:
        case MODULE_TYPE_R9M:
          required_protocol = PROTO_PXX;
          break;
        case MODULE_TYPE_SBUS:
          required_protocol = PROTO_SBUS;
          break;
#if defined(MULTIMODULE)
        case MODULE_TYPE_MULTIMODULE:
          required_protocol = PROTO_MULTIMODULE;
          break;
#endif
#if defined(DSM2)
        case MODULE_TYPE_DSM2:
          required_protocol = limit<uint8_t>(PROTO_DSM2_LP45, PROTO_DSM2_LP45+g_model.moduleData[EXTERNAL_MODULE].rfProtocol, PROTO_DSM2_DSMX);
          // The module is set to OFF during one second before BIND start
          {
            static tmr10ms_t bindStartTime = 0;
            if (moduleFlag[EXTERNAL_MODULE] == MODULE_BIND) {
              if (bindStartTime == 0) bindStartTime = get_tmr10ms();
              if ((tmr10ms_t)(get_tmr10ms() - bindStartTime) < 100) {
                required_protocol = PROTO_NONE;
                break;
              }
            }
            else {
              bindStartTime = 0;
            }
          }
          break;
#endif
#if defined(CROSSFIRE)
        case MODULE_TYPE_CROSSFIRE:
          required_protocol = PROTO_CROSSFIRE;
          break;
#endif
        default:
          required_protocol = PROTO_NONE;
          break;
      }
      break;
  }

  if (s_pulses_paused) {
    required_protocol = PROTO_NONE;
  }

#if 0
  // will need an EEPROM conversion
  if (moduleFlag[port] == MODULE_OFF) {
    required_protocol = PROTO_NONE;
  }
#endif

  return required_protocol;
}


void disablePort(uint8_t port) {
  switch (s_current_protocol[port]) { // stop existing protocol hardware
        case PROTO_PXX:
          disable_pxx(port);
          break;

  #if defined(DSM2)
        case PROTO_DSM2_LP45:
        case PROTO_DSM2_DSM2:
        case PROTO_DSM2_DSMX:
          disable_serial(port);
          break;
  #endif

  #if defined(CROSSFIRE)
        case PROTO_CROSSFIRE:
          disable_crossfire(port);
          break;
  #endif

  #if defined(MULTIMODULE)
        case PROTO_MULTIMODULE:
  #endif
        case PROTO_SBUS:
          disable_serial(port);
          break;

        case PROTO_PPM:
          disable_ppm(port);
          break;

        default:
          disable_no_pulses(port);
          break;
      }
}
void setup(uint8_t port, uint8_t required_protocol, bool init_needed)
{
  switch (required_protocol) {
    case PROTO_PXX:
      setupPulsesPXX(port);
      scheduleNextMixerCalculation(port, PXX_PERIOD);
      break;

    case PROTO_SBUS:
      setupPulsesSbus(port);
      scheduleNextMixerCalculation(port, SBUS_PERIOD);
      break;

#if defined(DSM2)
    case PROTO_DSM2_LP45:
    case PROTO_DSM2_DSM2:
    case PROTO_DSM2_DSMX:
      setupPulsesDSM2(port);
      scheduleNextMixerCalculation(port, DSM2_PERIOD);
      break;
#endif

#if defined(CROSSFIRE)
    case PROTO_CROSSFIRE:
      if (telemetryProtocol == PROTOCOL_PULSES_CROSSFIRE && !init_needed) {
        uint8_t * crossfire = modulePulsesData[port].crossfire.pulses;
        uint8_t len;
#if defined(LUA)
        if (outputTelemetryBufferTrigger != 0x00 && outputTelemetryBufferSize > 0) {
          memcpy(crossfire, outputTelemetryBuffer, outputTelemetryBufferSize);
          len = outputTelemetryBufferSize;
          outputTelemetryBufferTrigger = 0x00;
          outputTelemetryBufferSize = 0;
        }
        else
#endif
        {
          len = createCrossfireChannelsFrame(crossfire, &channelOutputs[g_model.moduleData[port].channelsStart]);
        }
        sportSendBuffer(crossfire, len);
      }
      scheduleNextMixerCalculation(port, CROSSFIRE_PERIOD);
      break;
#endif

#if defined(MULTIMODULE)
    case PROTO_MULTIMODULE:
      setupPulsesMultimodule(port);
      scheduleNextMixerCalculation(port, MULTIMODULE_PERIOD);
      break;
#endif

    case PROTO_PPM:
#if defined(PCBSKY9X)
    case PROTO_NONE:
#endif
      setupPulsesPPMModule(port);
      scheduleNextMixerCalculation(port, PPM_PERIOD(port));
      break;

    default:
      break;
  }
}
void init(uint8_t port, uint8_t required_protocol)
{
  switch (required_protocol) { // Start new protocol hardware here
    case PROTO_PXX:
      init_pxx(port);
      break;

#if defined(DSM2)
    case PROTO_DSM2_LP45:
    case PROTO_DSM2_DSM2:
    case PROTO_DSM2_DSMX:
      init_serial(port, DSM2_BAUDRATE, DSM2_PERIOD * 2000);
      break;
#endif

#if defined(CROSSFIRE)
    case PROTO_CROSSFIRE:
      init_crossfire(port);
      break;
#endif

#if defined(MULTIMODULE)
    case PROTO_MULTIMODULE:
      init_serial(port, MULTIMODULE_BAUDRATE, MULTIMODULE_PERIOD * 2000);
      break;
#endif

    case PROTO_SBUS:
      init_serial(port, SBUS_BAUDRATE, SBUS_PERIOD_HALF_US);
      break;

    case PROTO_PPM:
      init_ppm(port);
      break;

    default:
      init_no_pulses(port);
      break;
  }
}

#if defined (SHARED_MODULE_GPIO)
void setupPulses(uint8_t port)
{
  //skip internal module if GPIO is shared
  if(port != EXTERNAL_MODULE) return;
  bool init_needed[] = { false, false };
  uint8_t required_protocol[] = { getRequiredProtocol(INTERNAL_MODULE), getRequiredProtocol(EXTERNAL_MODULE)};

  heartbeat |= (HEART_TIMER_PULSES << EXTERNAL_MODULE);
  heartbeat |= (HEART_TIMER_PULSES << INTERNAL_MODULE);

  if(required_protocol[0] != PROTO_NONE && required_protocol[1] != PROTO_NONE)
  {
    //ignore internal if both enabled
    required_protocol[0] = PROTO_NONE;
  }

  for (port = INTERNAL_MODULE; port <= EXTERNAL_MODULE; port++)
  {

    if (s_current_protocol[port] != required_protocol[port])
    {
      init_needed[port] = true;
      disablePort(port);
      s_current_protocol[port] = required_protocol[port];
    }
    // Set up output data here
    setup(port, required_protocol[port], init_needed[port]);

  }

  //Single module can be active at once

  if(init_needed[INTERNAL_MODULE] && required_protocol[INTERNAL_MODULE] != PROTO_NONE)
  {
    init(INTERNAL_MODULE, required_protocol[INTERNAL_MODULE]);
  }
  else if(init_needed[EXTERNAL_MODULE] && required_protocol[EXTERNAL_MODULE] != PROTO_NONE)
  {
    init(EXTERNAL_MODULE, required_protocol[EXTERNAL_MODULE]);
  }
  else if(init_needed[INTERNAL_MODULE] || init_needed[EXTERNAL_MODULE])
  {
    init_no_pulses(EXTERNAL_MODULE);
  }
}
#else
void setupPulses(uint8_t port)
{
  bool init_needed = false;
  uint8_t required_protocol = getRequiredProtocol(port);

  heartbeat |= (HEART_TIMER_PULSES << port);

  if (s_current_protocol[port] != required_protocol) {
    init_needed = true;
    disablePort(port);
    s_current_protocol[port] = required_protocol;
  }

  // Set up output data here
  setup(port, required_protocol, init_needed);

  if (init_needed) {
    init(port, required_protocol);
  }
}
#endif

void setCustomFailsafe(uint8_t moduleIndex)
{
  if (moduleIndex < NUM_MODULES) {
    for (int ch=0; ch<MAX_OUTPUT_CHANNELS; ch++) {
      if (ch < g_model.moduleData[moduleIndex].channelsStart || ch >= NUM_CHANNELS(moduleIndex) + g_model.moduleData[moduleIndex].channelsStart) {
        g_model.moduleData[moduleIndex].failsafeChannels[ch] = 0;
      }
      else if (g_model.moduleData[moduleIndex].failsafeChannels[ch] < FAILSAFE_CHANNEL_HOLD) {
        g_model.moduleData[moduleIndex].failsafeChannels[ch] = channelOutputs[ch];
      }
    }
  }
}
