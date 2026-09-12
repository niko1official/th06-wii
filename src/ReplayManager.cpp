#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ReplayManager.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"
#include "utils.hpp"

#include <SDL2/SDL_endian.h>

ReplayManager *g_ReplayManager;

static inline u16 ReplayReadLE16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static inline u32 ReplayReadLE32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static inline void ReplayWriteNative16(u8 *p, u16 value)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    p[0] = (u8)(value >> 8);
    p[1] = (u8)value;
#else
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
#endif
}

static inline void ReplayWriteNative32(u8 *p, u32 value)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    p[0] = (u8)(value >> 24);
    p[1] = (u8)(value >> 16);
    p[2] = (u8)(value >> 8);
    p[3] = (u8)value;
#else
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
    p[2] = (u8)(value >> 16);
    p[3] = (u8)(value >> 24);
#endif
}

static inline void ReplayNormalize16(u8 *p)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    ReplayWriteNative16(p, ReplayReadLE16(p));
#else
    (void)p;
#endif
}

static inline void ReplayNormalize32(u8 *p)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    ReplayWriteNative32(p, ReplayReadLE32(p));
#else
    (void)p;
#endif
}

ZunResult ReplayManager::ValidateReplayData(const ReplayHeader *data, i32 fileSize)
{
    if (data == NULL || fileSize < (i32)sizeof(ReplayHeader))
    {
        return ZUN_ERROR;
    }

    u8 *raw = (u8 *)data;

    if (std::memcmp(raw, "T6RP", 4) != 0)
    {
        return ZUN_ERROR;
    }

    u8 obfOffset = raw[offsetof(ReplayHeader, key)];
    u8 *obfuscateCursor = raw + offsetof(ReplayHeader, rngValue3);
    const i32 obfuscatedSize = fileSize - (i32)offsetof(ReplayHeader, rngValue3);
    if (obfuscatedSize < 0)
    {
        return ZUN_ERROR;
    }

    for (i32 idx = 0; idx < obfuscatedSize; idx++, obfuscateCursor++)
    {
        *obfuscateCursor -= obfOffset;
        obfOffset += 7;
    }

    const u32 storedChecksum = ReplayReadLE32(raw + offsetof(ReplayHeader, checksum));

    u32 checksum = 0x3f000318;
    const u8 *checksumCursor = raw + offsetof(ReplayHeader, key);
    const i32 checksumSize = fileSize - (i32)offsetof(ReplayHeader, key);
    if (checksumSize < 0)
    {
        return ZUN_ERROR;
    }

    for (i32 idx = 0; idx < checksumSize; idx++, checksumCursor++)
    {
        checksum += *checksumCursor;
    }

    if (checksum != storedChecksum)
    {
        return ZUN_ERROR;
    }

    if (ReplayReadLE16(raw + offsetof(ReplayHeader, version)) != GAME_VERSION)
    {
        return ZUN_ERROR;
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    ReplayNormalize16(raw + offsetof(ReplayHeader, version));
    ReplayNormalize32(raw + offsetof(ReplayHeader, checksum));
    ReplayNormalize32(raw + offsetof(ReplayHeader, score));
    ReplayNormalize32(raw + offsetof(ReplayHeader, slowdownRate2));
    ReplayNormalize32(raw + offsetof(ReplayHeader, slowdownRate));
    ReplayNormalize32(raw + offsetof(ReplayHeader, slowdownRate3));

    u32 stageOffsets[7];
    for (i32 idx = 0; idx < 7; idx++)
    {
        stageOffsets[idx] =
            ReplayReadLE32(raw + offsetof(ReplayHeader, stageReplayDataOffsets) + idx * sizeof(u32));

        if (stageOffsets[idx] != 0 &&
            (stageOffsets[idx] < sizeof(ReplayHeader) || stageOffsets[idx] >= (u32)fileSize ||
             (stageOffsets[idx] & 3) != 0))
        {
            return ZUN_ERROR;
        }

        ReplayNormalize32(raw + offsetof(ReplayHeader, stageReplayDataOffsets) + idx * sizeof(u32));
    }

    for (i32 stage = 0; stage < 7; stage++)
    {
        const u32 stageOffset = stageOffsets[stage];
        if (stageOffset == 0)
        {
            continue;
        }

        u32 nextOffset = (u32)fileSize;
        for (i32 other = 0; other < 7; other++)
        {
            if (stageOffsets[other] > stageOffset && stageOffsets[other] < nextOffset)
            {
                nextOffset = stageOffsets[other];
            }
        }

        if (nextOffset <= stageOffset ||
            nextOffset - stageOffset < offsetof(StageReplayData, replayInputs) +
                                            2 * sizeof(ReplayDataInput))
        {
            return ZUN_ERROR;
        }

        const u32 stageSize = nextOffset - stageOffset;
        const u32 inputBytes = stageSize - offsetof(StageReplayData, replayInputs);
        if ((inputBytes % sizeof(ReplayDataInput)) != 0)
        {
            return ZUN_ERROR;
        }

        const u32 inputCount = inputBytes / sizeof(ReplayDataInput);
        if (inputCount > 53998)
        {
            return ZUN_ERROR;
        }

        u8 *stageData = raw + stageOffset;
        ReplayNormalize32(stageData + offsetof(StageReplayData, score));
        ReplayNormalize16(stageData + offsetof(StageReplayData, randomSeed));
        ReplayNormalize16(stageData + offsetof(StageReplayData, pointItemsCollected));

        for (u32 input = 0; input < inputCount; input++)
        {
            u8 *entry = stageData + offsetof(StageReplayData, replayInputs) +
                         input * sizeof(ReplayDataInput);
            ReplayNormalize32(entry + offsetof(ReplayDataInput, frameNum));
            ReplayNormalize16(entry + offsetof(ReplayDataInput, inputKey));
        }
    }
#endif

    return ZUN_SUCCESS;
}

ZunResult ReplayManager::RegisterChain(i32 isDemo, const char *replayFile)
{
    ReplayManager *replayMgr;

    if (g_Supervisor.framerateMultiplier < 0.99f && !isDemo)
    {
        return ZUN_SUCCESS;
    }
    g_Supervisor.framerateMultiplier = 1.0f;
    if (g_ReplayManager == NULL)
    {
        replayMgr = new ReplayManager();
        g_ReplayManager = replayMgr;
        replayMgr->replayData = NULL;
        replayMgr->isDemo = isDemo;
        replayMgr->replayFile = replayFile;
        switch (isDemo)
        {
        case false:
            replayMgr->calcChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdate);
            replayMgr->calcChain->addedCallback = (ChainAddedCallback)AddedCallback;
            replayMgr->calcChain->deletedCallback = (ChainDeletedCallback)DeletedCallback;
            replayMgr->drawChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnDraw);
            replayMgr->calcChain->arg = replayMgr;
            if (g_Chain.AddToCalcChain(replayMgr->calcChain, TH_CHAIN_PRIO_CALC_REPLAYMANAGER))
            {
                g_Chain.Cut(replayMgr->calcChain);
                return ZUN_ERROR;
            }
            replayMgr->calcChainDemoHighPrio = NULL;
            break;
        case true:
            replayMgr->calcChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdateDemoHighPrio);
            replayMgr->calcChain->addedCallback = (ChainAddedCallback)AddedCallbackDemo;
            replayMgr->calcChain->deletedCallback = (ChainDeletedCallback)DeletedCallback;
            replayMgr->drawChain = g_Chain.CreateElem((ChainCallback)ReplayManager::OnDraw);
            replayMgr->calcChain->arg = replayMgr;
            if (g_Chain.AddToCalcChain(replayMgr->calcChain, TH_CHAIN_PRIO_CALC_LOW_PRIO_REPLAYMANAGER_DEMO))
            {
                g_Chain.Cut(replayMgr->calcChain);
                return ZUN_ERROR;
            }
            replayMgr->calcChainDemoHighPrio = g_Chain.CreateElem((ChainCallback)ReplayManager::OnUpdateDemoLowPrio);
            replayMgr->calcChainDemoHighPrio->arg = replayMgr;
            g_Chain.AddToCalcChain(replayMgr->calcChainDemoHighPrio, TH_CHAIN_PRIO_CALC_HIGH_PRIO_REPLAYMANAGER_DEMO);
            break;
        }
        replayMgr->drawChain->arg = replayMgr;
        g_Chain.AddToDrawChain(replayMgr->drawChain, TH_CHAIN_PRIO_DRAW_REPLAYMANAGER);
    }
    else
    {
        switch (isDemo)
        {
        case false:
            AddedCallback(g_ReplayManager);
            break;
        case true:
            return AddedCallbackDemo(g_ReplayManager);
            break;
        }
    }
    return ZUN_SUCCESS;
}

#define TH_BUTTON_REPLAY_CAPTURE                                                                                       \
    (TH_BUTTON_SHOOT | TH_BUTTON_BOMB | TH_BUTTON_FOCUS | TH_BUTTON_SKIP | TH_BUTTON_DIRECTION)

ChainCallbackResult ReplayManager::OnUpdate(ReplayManager *mgr)
{
    u16 inputs;

    if (!g_GameManager.isInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    inputs = IS_PRESSED(TH_BUTTON_REPLAY_CAPTURE);
    if (inputs != mgr->replayInputs->inputKey)
    {
        mgr->replayInputs += 1;
        mgr->replayInputStageBookmarks[g_GameManager.currentStage - 1] = mgr->replayInputs + 1;
        mgr->replayInputs->frameNum = mgr->frameId;
        mgr->replayInputs->inputKey = inputs;
    }
    mgr->frameId += 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnUpdateDemoLowPrio(ReplayManager *mgr)
{
    if (mgr == NULL || mgr->replayInputs == NULL)
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
    }
    if (!g_GameManager.isInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (g_Gui.HasCurrentMsgIdx() && g_Gui.IsDialogueSkippable() && mgr->frameId % 3 != 2)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnUpdateDemoHighPrio(ReplayManager *mgr)
{
    if (mgr == NULL || mgr->replayInputs == NULL)
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
    }
    if (!g_GameManager.isInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    while (mgr->frameId >= mgr->replayInputs[1].frameNum)
    {
        mgr->replayInputs += 1;
    }
    g_CurFrameInput = IS_PRESSED(0xFFFFFFFF & ~TH_BUTTON_REPLAY_CAPTURE) | mgr->replayInputs->inputKey;
    g_IsEigthFrameOfHeldInput = 0;
    if (g_LastFrameInput == g_CurFrameInput)
    {
        if (30 <= g_NumOfFramesInputsWereHeld)
        {
            if (g_NumOfFramesInputsWereHeld % 8 == 0)
            {
                g_IsEigthFrameOfHeldInput = 1;
            }
            if (38 <= g_NumOfFramesInputsWereHeld)
            {
                g_NumOfFramesInputsWereHeld = 30;
            }
        }
        g_NumOfFramesInputsWereHeld++;
    }
    else
    {
        g_NumOfFramesInputsWereHeld = 0;
    }
    mgr->frameId += 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ReplayManager::OnDraw(ReplayManager *mgr)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

inline StageReplayData *AllocateStageReplayData(i32 size)
{
    return (StageReplayData *)std::malloc(size);
}

inline void ReleaseReplayData(void *data)
{
    return std::free(data);
}

inline void ReleaseStageReplayData(void *data)
{
    return std::free(data);
}

ZunResult ReplayManager::AddedCallback(ReplayManager *mgr)
{
    StageReplayData *stageReplayData;
    StageReplayData *oldStageReplayData;
    i32 idx;

    mgr->frameId = 0;
    if (mgr->replayData == NULL)
    {
        mgr->replayData = new ReplayData();
        mgr->replayData->header = new ReplayHeader();
        std::memcpy(&mgr->replayData->header->magic[0], "T6RP", 4);
        mgr->replayData->header->shottypeChara = g_GameManager.character * 2 + g_GameManager.shotType;
        mgr->replayData->header->version = 0x102;
        mgr->replayData->header->difficulty = g_GameManager.difficulty;
        std::memcpy(&mgr->replayData->header->name, "NO NAME", 4);
        for (idx = 0; idx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); idx += 1)
        {
            mgr->replayData->stageReplayData[idx] = NULL;
        }
    }
    else
    {
        oldStageReplayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 2];
        if (oldStageReplayData == NULL)
        {
            return ZUN_ERROR;
        }
        oldStageReplayData->score = g_GameManager.score;
    }
    if (mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] != NULL)
    {
        utils::DebugPrint2("error : replay.cpp");
    }
    mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] = AllocateStageReplayData(sizeof(StageReplayData));
    stageReplayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 1];
    stageReplayData->bombsRemaining = g_GameManager.bombsRemaining;
    stageReplayData->livesRemaining = g_GameManager.livesRemaining;
    stageReplayData->power = g_GameManager.currentPower;
    stageReplayData->rank = g_GameManager.rank;
    stageReplayData->pointItemsCollected = g_GameManager.pointItemsCollected;
    stageReplayData->randomSeed = g_GameManager.randomSeed;
    stageReplayData->powerItemCountForScore = g_GameManager.powerItemCountForScore;
    mgr->replayInputs = stageReplayData->replayInputs;
    mgr->replayInputs->frameNum = 0;
    mgr->replayInputs->inputKey = 0;
    mgr->unk44 = 0;
    return ZUN_SUCCESS;
}

ZunResult ReplayManager::AddedCallbackDemo(ReplayManager *mgr)
{
    i32 idx;
    StageReplayData *replayData;

    mgr->frameId = 0;
    if (mgr->replayData == NULL)
    {
        mgr->replayData = (ReplayData *)std::malloc(sizeof(ReplayData));

        mgr->replayData->header = (ReplayHeader *)FileSystem::OpenPath(mgr->replayFile, g_GameManager.demoMode == 0);
        if (ValidateReplayData(mgr->replayData->header, g_LastFileSize) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        for (idx = 0; idx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); idx += 1)
        {
            if (mgr->replayData->header->stageReplayDataOffsets[idx] != 0)
            {
                mgr->replayData->stageReplayData[idx] =
                    (StageReplayData *)(((u8 *)mgr->replayData->header) +
                                        mgr->replayData->header->stageReplayDataOffsets[idx]);
            }
            else
            {
                mgr->replayData->stageReplayData[idx] = NULL;
            }
        }
    }
    if (mgr->replayData->stageReplayData[g_GameManager.currentStage - 1] == NULL)
    {
        return ZUN_ERROR;
    }
    replayData = mgr->replayData->stageReplayData[g_GameManager.currentStage - 1];
    g_GameManager.character = mgr->replayData->header->shottypeChara / 2;
    g_GameManager.shotType = mgr->replayData->header->shottypeChara % 2;
    g_GameManager.difficulty = (Difficulty)mgr->replayData->header->difficulty;
    g_GameManager.pointItemsCollected = replayData->pointItemsCollected;
    g_Rng.Initialize(replayData->randomSeed);
    g_GameManager.rank = replayData->rank;
    g_GameManager.livesRemaining = replayData->livesRemaining;
    g_GameManager.bombsRemaining = replayData->bombsRemaining;
    g_GameManager.currentPower = replayData->power;
    mgr->replayInputs = replayData->replayInputs;
    g_GameManager.powerItemCountForScore = replayData->powerItemCountForScore;
    if (2 <= g_GameManager.currentStage && mgr->replayData->stageReplayData[g_GameManager.currentStage - 2] != NULL)
    {
        g_GameManager.guiScore = mgr->replayData->stageReplayData[g_GameManager.currentStage - 2]->score;
        g_GameManager.score = g_GameManager.guiScore;
    }
    return ZUN_SUCCESS;
}

ZunResult ReplayManager::DeletedCallback(ReplayManager *mgr)
{
    if (mgr == NULL)
    {
        return ZUN_SUCCESS;
    }

    g_Chain.Cut(mgr->drawChain);
    mgr->drawChain = NULL;

    if (mgr->calcChainDemoHighPrio != NULL)
    {
        g_Chain.Cut(mgr->calcChainDemoHighPrio);
        mgr->calcChainDemoHighPrio = NULL;
    }

    if (mgr->replayData != NULL)
    {
        std::free(mgr->replayData->header);
        ReleaseReplayData(mgr->replayData);
        mgr->replayData = NULL;
    }

    if (g_ReplayManager == mgr)
    {
        g_ReplayManager = NULL;
    }

    delete mgr;
    return ZUN_SUCCESS;
}

void ReplayManager::StopRecording()
{
    ReplayManager *mgr = g_ReplayManager;
    if (mgr != NULL && mgr->replayInputs != NULL && !mgr->IsDemo())
    {
        mgr->replayInputs += 1;
        mgr->replayInputs->frameNum = mgr->frameId;
        mgr->replayInputs->inputKey = 0;
        mgr->replayInputs += 1;
        mgr->replayInputs->frameNum = 9999999;
        mgr->replayInputs->inputKey = 0;
        mgr->replayInputStageBookmarks[g_GameManager.currentStage - 1] = mgr->replayInputs + 1;
    }
}

void ReplayManager::SaveReplay(const char *replayPath, char *replayName)
{
    ReplayManager *mgr;
    FILE *file;
    const u8 *checksumCursor;
    ReplayHeader replayCopy;
    u8 *obfuscateCursor;
    i32 obfStagePos;
    u8 obfOffset;
    u32 checksum;
    i32 csumStagePos;
    size_t stageReplayPos;
    f32 slowDown;
    i32 stageIdx;
    std::time_t time;
    const std::tm *tm;

    time = std::time(NULL);
    tm = std::localtime(&time);

    if (g_ReplayManager != NULL)
    {
        mgr = g_ReplayManager;
        if (!mgr->IsDemo())
        {
            if (replayPath != NULL)
            {
                replayCopy = *mgr->replayData->header;
                ReplayManager::StopRecording();
                stageReplayPos = sizeof(ReplayHeader);
                for (stageIdx = 0; stageIdx < ARRAY_SIZE_SIGNED(g_ReplayManager->replayData->stageReplayData);
                     stageIdx += 1)
                {
                    if (mgr->replayData->stageReplayData[stageIdx] != NULL)
                    {
                        replayCopy.stageReplayDataOffsets[stageIdx] = (u32)stageReplayPos;
                        stageReplayPos += (size_t)((u8 *)mgr->replayInputStageBookmarks[stageIdx] -
                                                   (u8 *)mgr->replayData->stageReplayData[stageIdx]);
                    }
                }
                utils::DebugPrint2("%s write ...\n", replayPath);
                replayCopy.score = g_GameManager.guiScore;
                slowDown = (g_Supervisor.unk1b4 / g_Supervisor.unk1b8 - 0.5f) * 2.0f;
                if (slowDown < 0.0f)
                {
                    slowDown = 0.0f;
                }
                else if (slowDown >= 1.0f)
                {
                    slowDown = 1.0f;
                }
                replayCopy.slowdownRate = (1.0f - slowDown) * 100.0f;
                replayCopy.slowdownRate2 = replayCopy.slowdownRate + 1.12f;
                replayCopy.slowdownRate3 = replayCopy.slowdownRate + 2.34f;
                mgr->replayData->stageReplayData[g_GameManager.currentStage - 1]->score = g_GameManager.score;
                std::strcpy(replayCopy.name, replayName);
                std::sprintf(replayCopy.date, "%02i/%02i/%02i", tm->tm_mon, tm->tm_mday, tm->tm_year % 100);
                replayCopy.key = g_Rng.GetRandomU16InRange(128) + 64;
                replayCopy.rngValue3 = g_Rng.GetRandomU16InRange(256);
                replayCopy.rngValue1 = g_Rng.GetRandomU16InRange(256);
                replayCopy.rngValue2 = g_Rng.GetRandomU16InRange(256);

                checksumCursor = (u8 *)&replayCopy.key;
                checksum = 0x3f000318;
                for (stageIdx = 0; stageIdx < sizeof(ReplayHeader) - offsetof(ReplayHeader, key);
                     stageIdx += 1, checksumCursor += 1)
                {
                    checksum += *checksumCursor;
                }
                for (stageIdx = 0; stageIdx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); stageIdx += 1)
                {
                    if (mgr->replayData->stageReplayData[stageIdx] != NULL)
                    {
                        checksumCursor = (u8 *)mgr->replayData->stageReplayData[stageIdx];
                        for (csumStagePos = 0; csumStagePos < ((iptr)mgr->replayInputStageBookmarks[stageIdx]) -
                                                                  ((iptr)mgr->replayData->stageReplayData[stageIdx]);
                             csumStagePos += 1, checksumCursor += 1)
                        {
                            checksum += *checksumCursor;
                        }
                    }
                }
                replayCopy.checksum = checksum;

                obfuscateCursor = (u8 *)&replayCopy.rngValue3;
                obfOffset = replayCopy.key;
                for (stageIdx = 0; stageIdx < sizeof(ReplayHeader) - offsetof(ReplayHeader, rngValue3);
                     stageIdx += 1, obfuscateCursor += 1)
                {
                    *obfuscateCursor += obfOffset;
                    obfOffset += 7;
                }
                for (stageIdx = 0; stageIdx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); stageIdx += 1)
                {
                    if (mgr->replayData->stageReplayData[stageIdx] != NULL)
                    {
                        obfuscateCursor = (u8 *)mgr->replayData->stageReplayData[stageIdx];
                        for (obfStagePos = 0; obfStagePos < ((iptr)mgr->replayInputStageBookmarks[stageIdx]) -
                                                                ((iptr)mgr->replayData->stageReplayData[stageIdx]);
                             obfStagePos += 1, obfuscateCursor += 1)
                        {
                            *obfuscateCursor += obfOffset;
                            obfOffset += 7;
                        }
                    }
                }

                file = FileSystem::FopenUTF8(replayPath, "wb");
                std::fwrite(&replayCopy, sizeof(ReplayHeader), 1, file);
                for (stageIdx = 0; stageIdx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); stageIdx += 1)
                {
                    if (mgr->replayData->stageReplayData[stageIdx] != NULL)
                    {
                        std::fwrite(mgr->replayData->stageReplayData[stageIdx], 1,
                                    ((iptr)mgr->replayInputStageBookmarks[stageIdx]) -
                                        ((iptr)mgr->replayData->stageReplayData[stageIdx]),
                                    file);
                    }
                }
                std::fclose(file);
            }
            for (stageIdx = 0; stageIdx < ARRAY_SIZE_SIGNED(mgr->replayData->stageReplayData); stageIdx += 1)
            {
                if (g_ReplayManager->replayData->stageReplayData[stageIdx] != NULL)
                {
                    utils::DebugPrint2("Replay Size %d\n", ((iptr)mgr->replayInputStageBookmarks[stageIdx]) -
                                                               ((iptr)mgr->replayData->stageReplayData[stageIdx]));
                    ReleaseStageReplayData(g_ReplayManager->replayData->stageReplayData[stageIdx]);
                }
            }
        }
        g_Chain.Cut(g_ReplayManager->calcChain);
    }
    return;
}
