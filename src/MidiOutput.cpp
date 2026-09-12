#include "MidiOutput.hpp"
#include "FileSystem.hpp"
#include "Supervisor.hpp"
#include "ZunMemory.hpp"
#include "i18n.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

#include <SDL2/SDL_endian.h>
#include <cstdlib>
#include <cstring>

void MidiOutput::StartTimer(u32 delay, SDL_TimerCallback cb, void *data)
{
    this->StopTimer();

    this->lastTimerTicks = SDL_GetTicks();

    if (cb != NULL)
    {
        this->timerId = SDL_AddTimer(delay, cb, data);
    }
    else
    {
        this->timerId = SDL_AddTimer(delay, (SDL_TimerCallback)&MidiOutput::DefaultTimerCallback, this);
    }
}

i32 MidiOutput::StopTimer()
{
    if (this->timerId != 0)
    {
        SDL_RemoveTimer(this->timerId);
    }

    this->timerId = 0;

    return 1;
}

u32 SDLCALL MidiOutput::DefaultTimerCallback(u32 interval, MidiOutput *timer)
{
    timer->OnTimerElapsed();

    return interval;
}

u32 MidiOutput::ReadVariableLength(u8 **curTrackDataCursor)
{
    u32 length;
    u8 tmp;

    length = 0;
    do
    {
        tmp = **curTrackDataCursor;
        *curTrackDataCursor = *curTrackDataCursor + 1;
        length = length * 0x80 + (tmp & 0x7f);
    } while ((tmp & 0x80) != 0);

    return length;
}

MidiOutput::MidiOutput()
{
    this->timerId = 0;

    this->tracks = NULL;
    this->divisions = 0;
    this->tempo = 0;
    this->numTracks = 0;
    this->fadeOutVolumeMultiplier = 0;
    this->fadeOutLastSetVolume = 0;
    this->fadeOutFlag = false;

    for (int i = 0; i < ARRAY_SIZE_SIGNED(this->midiFileData); i++)
    {
        this->midiFileData[i] = NULL;
    }
}

MidiOutput::~MidiOutput()
{
    this->StopTimer();

    this->StopPlayback();
    this->ClearTracks();
    for (i32 i = 0; i < 32; i++)
    {
        this->ReleaseFileData(i);
    }
}

ZunResult MidiOutput::ReadFileData(u32 idx, const char *path)
{
    if (g_Supervisor.cfg.musicMode != MIDI)
    {
        return ZUN_SUCCESS;
    }

    this->StopPlayback();
    this->ReleaseFileData(idx);

    this->midiFileData[idx] = FileSystem::OpenPath(path, false);

    if (this->midiFileData[idx] == NULL)
    {
        g_GameErrorContext.Log(TH_ERR_MIDI_FAILED_TO_READ_FILE, path);
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

void MidiOutput::ReleaseFileData(u32 idx)
{
    std::free(this->midiFileData[idx]);

    this->midiFileData[idx] = NULL;
}

void MidiOutput::ClearTracks()
{
    i32 trackIndex;
    u8 *data;
    MidiTrack *tracks;

    for (trackIndex = 0; trackIndex < this->numTracks; trackIndex++)
    {
        data = this->tracks[trackIndex].trackData;
        std::free(data);
    }

    tracks = this->tracks;
    std::free(tracks);
    this->tracks = NULL;
    this->numTracks = 0;
}

ZunResult MidiOutput::ParseFile(i32 fileIdx)
{
    u8 hdrRaw[8];
    u32 trackLength;
    const u8 *currentCursor;
    const u8 *currentCursorTrack;
    const u8 *endOfHeaderPointer;
    i32 trackIdx;
    u32 hdrLength;

    this->ClearTracks();
    currentCursor = this->midiFileData[fileIdx];
    if (currentCursor == NULL)
    {
        utils::DebugPrint2(TH_ERR_MIDI_NOT_LOADED);
        return ZUN_ERROR;
    }

    std::memcpy(&hdrRaw, currentCursor, 8);

    currentCursor += sizeof(hdrRaw);
    hdrLength = SDL_SwapBE32(*(u32 *)(hdrRaw + 4));

    endOfHeaderPointer = currentCursor;
    currentCursor += hdrLength;

    this->format = SDL_SwapBE16(*(u16 *)endOfHeaderPointer);

    this->divisions = SDL_SwapBE16(*(u16 *)(endOfHeaderPointer + 4));

    this->numTracks = SDL_SwapBE16(*(u16 *)(endOfHeaderPointer + 2));

    this->tracks = (MidiTrack *)ZunMemory::Alloc(sizeof(MidiTrack) * this->numTracks);
    std::memset(this->tracks, 0, sizeof(MidiTrack) * this->numTracks);
    for (trackIdx = 0; trackIdx < this->numTracks; trackIdx++)
    {
        currentCursorTrack = currentCursor;
        currentCursor += 8;

        trackLength = SDL_SwapBE32(*(u32 *)(currentCursorTrack + 4));
        this->tracks[trackIdx].trackLength = trackLength;
        this->tracks[trackIdx].trackData = (u8 *)ZunMemory::Alloc(trackLength);
        this->tracks[trackIdx].trackPlaying = 1;
        std::memcpy(this->tracks[trackIdx].trackData, currentCursor, trackLength);
        currentCursor += trackLength;
    }
    this->tempo = 1'000'000;
    return ZUN_SUCCESS;
}

ZunResult MidiOutput::LoadFile(const char *midiPath)
{
    if (this->ReadFileData(0x1f, midiPath) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    this->ParseFile(0x1f);
    this->ReleaseFileData(0x1f);

    return ZUN_SUCCESS;
}

void MidiOutput::LoadTracks()
{
    i32 trackIndex;
    MidiTrack *track = this->tracks;

    this->fadeOutVolumeMultiplier = 1.0;
    this->fadeOutFlag = false;
    this->elapsedMS = 0;
    this->tickBase = 0;

    for (trackIndex = 0; trackIndex < this->numTracks; trackIndex++, track++)
    {
        track->curTrackDataCursor = track->trackData;
        track->loopPointTarget = track->curTrackDataCursor;
        track->trackPlaying = true;
        track->nextMessageTimePos = MidiOutput::ReadVariableLength(&track->curTrackDataCursor);
    }
}

ZunResult MidiOutput::Play()
{
    if (this->tracks == NULL)
    {
        return ZUN_ERROR;
    }

    this->LoadTracks();
    this->midiOutDev.OpenDevice(0xFFFF'FFFF);
    this->StartTimer(1, NULL, NULL);

    return ZUN_SUCCESS;
}

ZunResult MidiOutput::StopPlayback()
{
    if (this->tracks == NULL)
    {
        return ZUN_ERROR;
    }
    else
    {
        this->StopTimer();
        this->midiOutDev.Close();

        return ZUN_SUCCESS;
    }
}

u32 MidiOutput::SetFadeOut(u32 ms)
{
    this->fadeOutVolumeMultiplier = 0.0;
    this->fadeOutInterval = ms;
    this->fadeOutElapsedMS = 0;
    this->fadeOutFlag = true;

    return 0;
}

void MidiOutput::OnTimerElapsed()
{
    u64 timePos;
    i32 trackIndex;
    bool trackLoaded;

    trackLoaded = false;
    timePos = this->tickBase + (this->elapsedMS * this->divisions * 1000) / this->tempo;
    if (this->fadeOutFlag)
    {
        if (this->fadeOutElapsedMS < this->fadeOutInterval)
        {
            this->fadeOutVolumeMultiplier = 1.0f - (f32)this->fadeOutElapsedMS / (f32)this->fadeOutInterval;
            if ((u32)(this->fadeOutVolumeMultiplier * 128.0f) != this->fadeOutLastSetVolume)
            {
                this->FadeOutSetVolume(0);
            }
            this->fadeOutLastSetVolume = this->fadeOutVolumeMultiplier * 128.0f;
            this->fadeOutElapsedMS = this->fadeOutElapsedMS + 1;
        }
        else
        {
            this->fadeOutVolumeMultiplier = 0.0;
            return;
        }
    }

    for (trackIndex = 0; trackIndex < this->numTracks; trackIndex++)
    {
        if (this->tracks[trackIndex].trackPlaying)
        {
            trackLoaded = true;
            while (this->tracks[trackIndex].trackPlaying)
            {
                if (this->tracks[trackIndex].nextMessageTimePos <= timePos)
                {
                    this->ProcessMsg(&this->tracks[trackIndex]);
                    timePos = this->tickBase + (this->elapsedMS * this->divisions * 1000 / this->tempo);
                    continue;
                }
                break;
            }
        }
    }

    u32 curTicks = SDL_GetTicks();
    this->elapsedMS += curTicks - this->lastTimerTicks;
    this->lastTimerTicks = curTicks;

    if (!trackLoaded)
    {
        this->LoadTracks();
    }
}

void MidiOutput::ProcessMsg(MidiTrack *track)
{
    i32 curTrackLength;
    u8 arg1, arg2;
    u8 opcode, opcodeHigh, opcodeLow;
    u8 metaEventID;
    i32 idx;
    u8 *sysExData;

    opcode = *track->curTrackDataCursor;
    if (opcode < MIDI_OPCODE_NOTE_OFF)
    {
        opcode = track->opcode;
    }
    else
    {
        track->curTrackDataCursor++;
    }

    opcodeHigh = opcode & 0xf0;
    opcodeLow = opcode & 0x0f;
    switch (opcodeHigh)
    {
    case MIDI_OPCODE_SYSTEM_EXCLUSIVE:
        if (opcode == MIDI_OPCODE_SYSTEM_EXCLUSIVE)
        {
            curTrackLength = MidiOutput::ReadVariableLength(&track->curTrackDataCursor);

            sysExData = (u8 *)std::malloc(curTrackLength + 1);
            sysExData[0] = MIDI_OPCODE_SYSTEM_EXCLUSIVE;

            std::memcpy(sysExData + 1, track->curTrackDataCursor, curTrackLength);

            this->midiOutDev.SendLongMsg(sysExData, curTrackLength + 1);

            track->curTrackDataCursor += curTrackLength;

            std::free(sysExData);
        }
        else if (opcode == MIDI_OPCODE_SYSTEM_RESET)
        {

            metaEventID = *track->curTrackDataCursor;
            track->curTrackDataCursor++;
            curTrackLength = MidiOutput::ReadVariableLength(&track->curTrackDataCursor);

            if (metaEventID == 0x2f)
            {
                track->trackPlaying = false;
                return;
            }

            if (metaEventID == 0x51)
            {
                this->tickBase += (this->elapsedMS * this->divisions * 1000 / this->tempo);
                this->elapsedMS = 0;
                this->tempo = 0;

                for (idx = 0; idx < curTrackLength; idx++)
                {
                    this->tempo += this->tempo * 0x100 + *track->curTrackDataCursor;
                    track->curTrackDataCursor++;
                }

                break;
            }

            track->curTrackDataCursor += curTrackLength;
        }
        break;
    case MIDI_OPCODE_NOTE_OFF:
    case MIDI_OPCODE_NOTE_ON:
    case MIDI_OPCODE_POLYPHONIC_AFTERTOUCH:
    case MIDI_OPCODE_MODE_CHANGE:
    case MIDI_OPCODE_PITCH_BEND_CHANGE:
        arg1 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        arg2 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        break;
    case MIDI_OPCODE_PROGRAM_CHANGE:
    case MIDI_OPCODE_CHANNEL_AFTERTOUCH:
        arg1 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        arg2 = 0;
        break;
    }

    switch (opcodeHigh)
    {
    case MIDI_OPCODE_NOTE_ON:
        if (arg2 != 0)
        {
            this->channels[opcodeLow].keyPressedFlags[arg1 >> 3] |= ZUN_BIT(arg1 & 7);
            break;
        }

        SDL_FALLTHROUGH;
    case MIDI_OPCODE_NOTE_OFF:
        this->channels[opcodeLow].keyPressedFlags[arg1 >> 3] &= ~(ZUN_BIT(arg1 & 7));
        break;
    case MIDI_OPCODE_PROGRAM_CHANGE:

        this->channels[opcodeLow].instrument = arg1;
        break;
    case MIDI_OPCODE_MODE_CHANGE:
        switch (arg1)
        {
        case 0:

            this->channels[opcodeLow].instrumentBank = arg2;
            break;
        case 7:

            this->channels[opcodeLow].channelVolume = arg2;
            break;
        case 91:

            this->channels[opcodeLow].effectOneDepth = arg2;
            break;
        case 93:

            this->channels[opcodeLow].effectThreeDepth = arg2;
            break;
        case 10:

            this->channels[opcodeLow].pan = arg2;
            break;

        case 2:

            for (i32 i = 0; i < this->numTracks; i++)
            {
                this->tracks[i].loopPointTarget = this->tracks[i].curTrackDataCursor;
                this->tracks[i].loopPointTimePos = this->tracks[i].nextMessageTimePos;
            }
            this->loopPointTempo = this->tempo;
            this->loopPointMSCount = this->elapsedMS;
            this->loopPointBaseTicks = this->tickBase;

            break;
        case 4:

            for (i32 i = 0; i < this->numTracks; i++)
            {
                this->tracks[i].curTrackDataCursor = this->tracks[i].loopPointTarget;
                this->tracks[i].nextMessageTimePos = this->tracks[i].loopPointTimePos;
            }
            this->tempo = this->loopPointTempo;
            this->elapsedMS = this->loopPointMSCount;
            this->tickBase = this->loopPointBaseTicks;

            break;
        }
        break;
    }

    if (opcode < MIDI_OPCODE_SYSTEM_EXCLUSIVE)
    {
        this->midiOutDev.SendShortMsg(opcode, arg1, arg2);
    }

    track->opcode = opcode;
    track->nextMessageTimePos += MidiOutput::ReadVariableLength(&track->curTrackDataCursor);
}

void MidiOutput::FadeOutSetVolume(i32 volume)
{
    i32 idx;
    i32 volumeClamped;

    for (idx = 0; idx < ARRAY_SIZE_SIGNED(this->channels); idx++)
    {
        volumeClamped = (i32)(this->channels[idx].channelVolume * this->fadeOutVolumeMultiplier) + volume;

        if (volumeClamped < 0)
        {
            volumeClamped = 0;
        }
        else if (volumeClamped > 127)
        {
            volumeClamped = 127;
        }

        this->midiOutDev.SendShortMsg(MIDI_OPCODE_MODE_CHANGE | idx, 7, volumeClamped);
    }
}
