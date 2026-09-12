#pragma once

#include "ZunResult.hpp"
#include "inttypes.hpp"
#include <mmsystem.h>
#include <windows.h>

struct MidiDevice
{
  public:
    MidiDevice();
    ~MidiDevice();

    ZunResult Close();
    bool OpenDevice(u32 uDeviceId);
    bool SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte);
    bool SendLongMsg(const u8 *buf, u32 len);

  private:
    ZunResult UnprepareHeader(LPMIDIHDR pmh);

    HMIDIOUT handle;
    u32 deviceId;

    MIDIHDR *midiHeaders[32];
    u32 midiHeadersCursor;
};
