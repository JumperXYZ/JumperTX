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
#include "telemetry.h"
#include "multi.h"

enum MultiPacketTypes : uint8_t {
  MultiStatus = 1,
  FrSkySportTelemtry,
  FrSkyHubTelemetry,
  SpektrumTelemetry,
  DSMBindPacket,
  FlyskyIBusTelemetry,
  ConfigCommand,
  InputSync,
  FrskySportPolling
};

enum MultiBufferState : uint8_t {
  NoProtocolDetected,
  MultiFirstByteReceived,
  ReceivingMultiProtocol,
  ReceivingMultiStatus,
  SpektrumTelemetryFallback,
  FrskyTelemetryFallback,
  FrskyTelemetryFallbackFirstByte,
  FrskyTelemetryFallbackNextBytes,
  FlyskyTelemetryFallback,
  MultiStatusOrFrskyData
};

//implement getters by port

MultiModuleStatus multiModuleStatus;
MultiModuleSyncStatus multiSyncStatus;
uint8_t multiBindStatus[NUM_MODULES] = { MULTI_NORMAL_OPERATION, MULTI_NORMAL_OPERATION };

static MultiBufferState multiTelemetryBufferState[NUM_MODULES];
#if defined(INTERNAL_MULTIMODULE)
uint8_t intTelemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];
uint8_t intTelemetryRxBufferCount;

MultiModuleStatus intMultiModuleStatus;
MultiModuleSyncStatus intMultiSyncStatus;
#endif






MultiBufferState guessProtocol(uint8_t port)
{
  if (g_model.moduleData[port].getMultiProtocol(false) == MM_RF_PROTO_DSM2)
    return SpektrumTelemetryFallback;
  else if (g_model.moduleData[port].getMultiProtocol(false) == MM_RF_PROTO_FS_AFHDS2A)
    return FlyskyTelemetryFallback;
  else
    return FrskyTelemetryFallback;
}

static void processMultiStatusPacket(const uint8_t *data, uint8_t port)
{
  MultiModuleStatus* status = &multiModuleStatus;
#if defined(INTERNAL_MULTIMODULE)
  if(port == INTERNAL_MODULE) status = &intMultiModuleStatus;
#endif

  // At least two status packets without bind flag
  bool wasBinding = ((*status).isBinding());

  (*status).flags = data[0];
  (*status).major = data[1];
  (*status).minor = data[2];
  (*status).revision = data[3];
  (*status).patch = data[4];
  (*status).lastUpdate = get_tmr10ms();

  if (wasBinding && !(*status).isBinding() && multiBindStatus[port] == MULTI_BIND_INITIATED)
    multiBindStatus[port] = MULTI_BIND_FINISHED;
}

static void processMultiSyncPacket(const uint8_t *data, uint8_t port)
{
  MultiModuleSyncStatus *syncStatus = &multiSyncStatus;
#if defined(INTERNAL_MULTIMODULE)
  if(port == INTERNAL_MODULE) syncStatus = &intMultiSyncStatus;
#endif
  (*syncStatus).lastUpdate = get_tmr10ms();
  (*syncStatus).interval = data[4];
  (*syncStatus).target = data[5];
#if !defined(PPM_PIN_SERIAL)
  auto oldlag = (*syncStatus).inputLag;
  (void) oldlag;
#endif

  (*syncStatus).calcAdjustedRefreshRate(data[0] << 8 | data[1], data[2] << 8 | data[3]);

#if !defined(PPM_PIN_SERIAL)
  TRACE("MP ADJ: rest: %d, lag %04d, diff: %04d  target: %d, interval: %d, Refresh: %d, intAdjRefresh: %d, adjRefresh %d\r\n", modulePulsesData[port].dsm2.rest,
      (*syncStatus).inputLag, oldlag-multiSyncStatus.inputLag, (*syncStatus).target, (*syncStatus).interval, (*syncStatus).refreshRate, (*syncStatus).adjustedRefreshRate/50,
      (*syncStatus).getAdjustedRefreshRate());
#endif
}


static void processMultiTelemetryPaket(const uint8_t *packet, uint8_t port)
{
  uint8_t type = packet[0];
  uint8_t len = packet[1];
  const uint8_t *data = packet + 2;

  // Switch type
  switch (type) {
    case MultiStatus:
      if (len >= 5)
        processMultiStatusPacket(data, port);
      break;
    case DSMBindPacket:
      if (len >= 10)
        processDSMBindPacket(data, port);
      break;
    case SpektrumTelemetry:
      // processSpektrumPacket expects data[0] to be the telemetry indicator 0xAA but does not check it,
      // just send one byte of our header instead
      if (len >= 17)
        processSpektrumPacket(data - 1);
      else
        TRACE("[MP] Received spektrum telemetry len %d < 17", len);
      break;
    case FlyskyIBusTelemetry:
      if (len >= 28)
        processFlySkyPacket(data);
      else
        TRACE("[MP] Received IBUS telemetry len %d < 28", len);
      break;
    case FrSkyHubTelemetry:
      if (len >= 4)
        frskyDProcessPacket(data);
      else
        TRACE("[MP] Received Frsky HUB telemetry len %d < 4", len);
      break;
    case FrSkySportTelemtry:
      if (len >= 4)
        sportProcessTelemetryPacket(data);
      else
        TRACE("[MP] Received sport telemetry len %d < 4", len);
      break;
    case InputSync:
      if (len >= 6)
        processMultiSyncPacket(data, port);
      else
        TRACE("[MP] Received input sync len %d < 6", len);
      break;
    case ConfigCommand:
      // Just an ack to our command, ignore for now
      break;
    case FrskySportPolling:
      #if defined(LUA)
      if (len >= 1 && outputTelemetryBufferSize > 0 && data[0] == outputTelemetryBufferTrigger) {
        TRACE("MP Sending sport data out.");
        sportSendBuffer(outputTelemetryBuffer, outputTelemetryBufferSize);
      }
      #endif
      break;
    default:
      TRACE("[MP] Unkown multi packet type 0x%02X, len %d", type, len);
      break;
  }
}

// sprintf does not work AVR ARM
// use a small helper function
static void appendInt(char *buf, uint32_t val)
{
  while (*buf)
    buf++;

  strAppendUnsigned(buf, val);
}

#define MIN_REFRESH_RATE      7000

void MultiModuleSyncStatus::calcAdjustedRefreshRate(uint16_t newRefreshRate, uint16_t newInputLag)
{
  // Check how far off we are from our target, positive means we are too slow, negative we are too fast
  int lagDifference = newInputLag - inputLag;

  // The refresh rate that we target
  // Below is least common multiple of MIN_REFRESH_RATE and requested rate
  uint16_t targetRefreshRate = (uint16_t) (newRefreshRate * ((MIN_REFRESH_RATE / (newRefreshRate - 1)) + 1));

  // Overflow, reverse sample
  if (lagDifference < -targetRefreshRate/2)
    lagDifference= -lagDifference;


  // Reset adjusted refresh if rate has changed
  if (newRefreshRate != refreshRate) {
    refreshRate = newRefreshRate;
    adjustedRefreshRate = targetRefreshRate;
    if (adjustedRefreshRate >= 30000)
      adjustedRefreshRate /= 2;

    // Our refresh rate in ps
    adjustedRefreshRate*=1000;
    return;
  }

  // Caluclate how many samples went into the reported input Lag (*10)
  int numsamples = interval * 10000 / targetRefreshRate;

  // Convert lagDifference to ps
  lagDifference=lagDifference*1000;

  // Calculate the time we intentionally were late/early
  if (inputLag > target*10 +30)
   lagDifference += numsamples*500;
  else if (inputLag < target*10 - 30)
    lagDifference -= numsamples*500;

  // Caculate the time in ps each frame is to slow (positive), fast(negative)
  int perframeps = lagDifference*10/ numsamples;

  if (perframeps > 20000)
    perframeps = 20000;

  if (perframeps < -20000)
    perframeps = -20000;

  adjustedRefreshRate =(adjustedRefreshRate + perframeps);

  // Safeguards
  if (adjustedRefreshRate < 6*1000*1000)
    adjustedRefreshRate = 6*1000*1000;
  if (adjustedRefreshRate > 30*1000*1000)
    adjustedRefreshRate = 30*1000*1000;

  inputLag = newInputLag;
}

static uint8_t counter;

uint16_t MultiModuleSyncStatus::getAdjustedRefreshRate() {
  if (!isValid() || refreshRate == 0)
    return 18000;


  counter = (uint8_t) (counter + 1 % 10);
  uint16_t rate = (uint16_t) ((adjustedRefreshRate + counter * 50) / 500);
  // Check how far off we are from our target, positive means we are too slow, negative we are too fast
 if (inputLag > target*10 +30)
     return (uint16_t) (rate - 1);
  else if (inputLag < target*10 - 30)
    return (uint16_t) (rate + 1);
  else
    return rate;
}


static void prependSpaces(char * buf, int val)
{
  while (*buf)
    buf++;

  int k=10000;
  while(val/k==0 && k > 0)
  {
    *buf=' ';
    buf++;
    k/= 10;
  }
  *buf='\0';
}

void MultiModuleSyncStatus::getRefreshString(char *statusText)
{
  if (!isValid()) {
    return;
  }

  strcpy(statusText, "L ");
  prependSpaces(statusText, inputLag);
  appendInt(statusText, inputLag);
  strcat(statusText, "ns R ");
  prependSpaces(statusText, adjustedRefreshRate/1000);
  appendInt(statusText, (uint32_t) (adjustedRefreshRate / 1000));
  strcat(statusText, "ns");
}

void MultiModuleStatus::getStatusString(char *statusText)
{
  if (!isValid()) {
#if defined(PCBTARANIS) || defined(PCBHORUS)
#if !defined(INTERNAL_MULTIMODULE)
    if (IS_INTERNAL_MODULE_ENABLED())
      strcpy(statusText, STR_DISABLE_INTERNAL);
    else
#endif
#endif
      strcpy(statusText, STR_MODULE_NO_TELEMETRY);
    return;
  }
  if (!protocolValid()) {
    strcpy(statusText, STR_PROTOCOL_INVALID);
    return;
  }
  else if (!serialMode()) {
    strcpy(statusText, STR_MODULE_NO_SERIAL_MODE);
    return;
  }
  else if (!inputDetected()) {
    strcpy(statusText, STR_MODULE_NO_INPUT);
    return;
  }
  else if(isWaitingforBind()) {
    strcpy(statusText, STR_MODULE_WAITFORBIND);
    return;
  }


  strcpy(statusText, "V");
  appendInt(statusText, major);
  strcat(statusText, ".");
  appendInt(statusText, minor);
  strcat(statusText, ".");
  appendInt(statusText, revision);
  strcat(statusText, ".");
  appendInt(statusText, patch);
  strcat(statusText, " ");

  if (isBinding())
    strcat(statusText, STR_MODULE_BINDING);
}


static void processMultiTelemetryByte(const uint8_t data, uint8_t module)
{
  uint8_t* rxBuffer = telemetryRxBuffer;
  uint8_t* rxBufferCount = &telemetryRxBufferCount;
#if defined(INTERNAL_MULTIMODULE)
  if(module == INTERNAL_MODULE) {
    rxBuffer = intTelemetryRxBuffer;
    rxBufferCount = &intTelemetryRxBufferCount;
  }
#endif
  if ((*rxBufferCount) < TELEMETRY_RX_PACKET_SIZE) {
    rxBuffer[(*rxBufferCount)++] = data;
  }
  else {
    TRACE("[MP] array size %d error", (*rxBufferCount));
    multiTelemetryBufferState[module] = NoProtocolDetected;
  }

  // Length field does not count the header
  if ((*rxBufferCount) >= 2 && rxBuffer[1] == (*rxBufferCount) - 2) {
    // debug print the content of the packet
#if 0
    debugPrintf("[MP] Packet type %02X len 0x%02X: ",
        rxBuffer[0], rxBuffer[1]);
    for (int i=0; i<((*rxBufferCount)+3)/4; i++) {
      debugPrintf("[%02X%02X %02X%02X] ", rxBuffer[i*4+2], rxBuffer[i*4 + 3],
          rxBuffer[i*4 + 4], rxBuffer[i*4 + 5]);
    }
    debugPrintf("\r\n");
#endif
    // Packet is complete, process it
    processMultiTelemetryPaket(rxBuffer, module);
    multiTelemetryBufferState[module] = NoProtocolDetected;
  }
}

void processMultiTelemetryData(const uint8_t data, uint8_t module)
{
  uint8_t* rxBuffer = telemetryRxBuffer;
  uint8_t* rxBufferCount = &telemetryRxBufferCount;
#if defined(INTERNAL_MULTIMODULE)
  if(module == INTERNAL_MODULE) {
    rxBuffer = intTelemetryRxBuffer;
    rxBufferCount = &intTelemetryRxBufferCount;
  }
#endif

  // debugPrintf("State: %d, byte received %02X, buflen: %d\r\n", multiTelemetryBufferState[module], data, *rxBufferCount);
  switch (multiTelemetryBufferState[module]) {
    case NoProtocolDetected:
      if (data == 'M') {
        multiTelemetryBufferState[module] = MultiFirstByteReceived;
      }
      else if (data == 0xAA || data == 0x7e) {
        multiTelemetryBufferState[module] = guessProtocol(module);

        // Process the first byte by the protocol
        processMultiTelemetryData(data, module);
      }
      else {
        TRACE("[MP] invalid start byte 0x%02X", data);
      }
      break;

    case FrskyTelemetryFallback:
      multiTelemetryBufferState[module] = FrskyTelemetryFallbackFirstByte;
      processFrskyTelemetryData(data);
      break;

    case FrskyTelemetryFallbackFirstByte:
      if (data == 'M') {
        multiTelemetryBufferState[module] = MultiStatusOrFrskyData;
      }
      else {
        processFrskyTelemetryData(data);
        if (data != 0x7e)
          multiTelemetryBufferState[module] = FrskyTelemetryFallbackNextBytes;
      }

      break;

    case FrskyTelemetryFallbackNextBytes:
      processFrskyTelemetryData(data);
      if (data == 0x7e)
        // end of packet or start of new packet
        multiTelemetryBufferState[module] = FrskyTelemetryFallbackFirstByte;
      break;

    case FlyskyTelemetryFallback:
      processFlySkyTelemetryData(data, rxBuffer, rxBufferCount);
      if ((*rxBufferCount) == 0)
        multiTelemetryBufferState[module] = NoProtocolDetected;
      break;

    case SpektrumTelemetryFallback:
      processSpektrumTelemetryData(data, module, rxBuffer, rxBufferCount);
      if ((*rxBufferCount) == 0)
        multiTelemetryBufferState[module] = NoProtocolDetected;
      break;

    case MultiFirstByteReceived:
      *rxBufferCount = 0;
      if (data == 'P') {
        multiTelemetryBufferState[module] = ReceivingMultiProtocol;
      }
      else if (data >= 5 && data <= 10) {
        // Protocol indented for er9x/ersky9, accept only 5-10 as packet length to have
        // a bit of validation
        multiTelemetryBufferState[module] = ReceivingMultiStatus;
        processMultiTelemetryData(data, module);
      }
      else {
        TRACE("[MP] invalid second byte 0x%02X", data);
        multiTelemetryBufferState[module] = NoProtocolDetected;
      }
      break;

    case ReceivingMultiProtocol:
      processMultiTelemetryByte(data, module);
      break;

    case MultiStatusOrFrskyData:
      // Check len byte if it makes sense for multi
      if (data >= 5 && data <= 10) {
        multiTelemetryBufferState[module] = ReceivingMultiStatus;
        *rxBufferCount = 0;
      }
      else {
        multiTelemetryBufferState[module] = FrskyTelemetryFallbackNextBytes;
        processMultiTelemetryData('M', module);
      }
      processMultiTelemetryData(data, module);
      break;

    case ReceivingMultiStatus:
      rxBuffer[(*rxBufferCount)++] = data;
      if ((*rxBufferCount) > 5 && rxBuffer[0] == (*rxBufferCount)-1) {
        processMultiStatusPacket(rxBuffer+1, module);
        *rxBufferCount = 0;
        multiTelemetryBufferState[module] = NoProtocolDetected;
      }
      if ((*rxBufferCount) > 10) {
        // too long ignore
        TRACE("Overlong multi status packet detected ignoring, wanted %d", rxBuffer[0]);
        *rxBufferCount = 0;
        multiTelemetryBufferState[module] = NoProtocolDetected;
      }


  }

}

