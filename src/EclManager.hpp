#pragma once

#include "ItemManager.hpp"
#include "SoundPlayer.hpp"
#include "ZunColor.hpp"
#include "ZunEndian.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

struct Enemy;
struct EnemyEclContext;
struct EnemyManager;

enum EclVarId : i32
{
    ECL_VAR_I32_0 = -10001,
    ECL_VAR_I32_1 = -10002,
    ECL_VAR_I32_2 = -10003,
    ECL_VAR_I32_3 = -10004,
    ECL_VAR_F32_0 = -10005,
    ECL_VAR_F32_1 = -10006,
    ECL_VAR_F32_2 = -10007,
    ECL_VAR_F32_3 = -10008,
    ECL_VAR_I32_4 = -10009,
    ECL_VAR_I32_5 = -10010,
    ECL_VAR_I32_6 = -10011,
    ECL_VAR_I32_7 = -10012,
    ECL_VAR_DIFFICULTY = -10013,
    ECL_VAR_RANK = -10014,
    ECL_VAR_ENEMY_POS_X = -10015,
    ECL_VAR_ENEMY_POS_Y = -10016,
    ECL_VAR_ENEMY_POS_Z = -10017,
    ECL_VAR_PLAYER_POS_X = -10018,
    ECL_VAR_PLAYER_POS_Y = -10019,
    ECL_VAR_PLAYER_POS_Z = -10020,
    ECL_VAR_PLAYER_ANGLE = -10021,
    ECL_VAR_ENEMY_TIMER = -10022,
    ECL_VAR_PLAYER_DISTANCE = -10023,
    ECL_VAR_ENEMY_LIFE = -10024,
    ECL_VAR_PLAYER_SHOT = -10025,
};

struct EclTimelineInstrArgs
{
    LE<u32> uintVar1;
    LE<u32> uintVar2;
    LE<u32> uintVar3;
    LE<u16> ushortVar1;
    LE<u16> ushortVar2;
    LE<u32> uintVar4;

    const ZunVec3Raw *Var1AsVec() const
    {
        return (const ZunVec3Raw *)&this->uintVar1;
    }
};

struct EclTimelineInstr
{
    LE<i16> time;
    LE<i16> arg0;
    LE<i16> opCode;
    LE<i16> size;
    EclTimelineInstrArgs args;
};

union EclRawInstrArg {
    struct
    {
        i8 a;
        i8 b;
        i8 c;
        i8 d;
    } by;
    struct
    {
        LE<i16> lo;
        LE<i16> hi;
    } sh;
    LE<f32> f32Param;
    LE<i32> i32Param;
    LE<EclVarId> id;
};

struct EclRawInstrAluArgs
{
    LE<EclVarId> res;
    EclRawInstrArg arg1;
    EclRawInstrArg arg2;
    EclRawInstrArg arg3;
    EclRawInstrArg arg4;
};

struct EclRawInstrJumpArgs
{
    LE<i32> time;
    LE<i32> offset;
    LE<EclVarId> var;
};

struct EclRawInstrCallArgs
{
    LE<i32> eclSub;
    LE<i32> var0;
    LE<f32> float0;
    LE<EclVarId> cmpLhs;
    LE<i32> cmpRhs;
};

struct EclRawInstrCmpArgs
{
    EclRawInstrArg lhs;
    EclRawInstrArg rhs;
};

struct EclRawInstrMoveArgs
{
    ZunVec3Raw pos;
};

struct EclRawInstrAnmSetMainArgs
{
    LE<i32> scriptIdx;
};

struct EclRawInstrAnmSetSlotArgs
{
    LE<i32> vmIdx;
    LE<i32> scriptIdx;
};

struct EclRawInstrAnmSetDeathArgs
{
    i8 deathAnm1;
    i8 deathAnm2;
    i8 deathAnm3;
};

struct EclRawInstrBulletArgs
{
    LE<i16> sprite;
    LE<i16> color;
    LE<EclVarId> count1;
    LE<EclVarId> count2;
    LE<f32> speed1;
    LE<f32> speed2;
    LE<f32> angle1;
    LE<f32> angle2;
    LE<i32> flags;
};

struct EclRawInstrLaserArgs
{
    LE<i16> sprite;
    LE<i16> color;
    LE<f32> angle;
    LE<f32> speed;
    LE<f32> startOffset;
    LE<f32> endOffset;
    LE<f32> startLength;
    LE<f32> width;
    LE<i32> startTime;
    LE<i32> duration;
    LE<i32> stopTime;
    LE<i32> grazeDelay;
    LE<i32> grazeDistance;
    LE<i32> flags;
};

struct EclRawInstrLaserOpArgs
{
    LE<i32> laserIdx;
    ZunVec3Raw arg1;
};

struct EclRawInstrBulletEffectsArgs
{
    LE<EclVarId> ivar1;
    LE<EclVarId> ivar2;
    LE<EclVarId> ivar3;
    LE<EclVarId> ivar4;
    LE<f32> fvar1;
    LE<f32> fvar2;
    LE<f32> fvar3;
    LE<f32> fvar4;
};

struct EclRawInstrSetInt
{
    LE<i32> i32Param;
};

struct EclRawInstrSpellcardEffectArgs
{
    LE<i32> effectColorId;
    ZunVec3Raw pos;
    LE<f32> effectDistance;
};

struct EclRawInstrMoveBoundSetArgs
{
    ZunVec2Raw lowerMoveLimit;
    ZunVec2Raw upperMoveLimit;
};

struct EclRawInstrAnmSetPosesArgs
{
    LE<i16> anmExDefault;
    LE<i16> anmExFarLeft;
    LE<i16> anmExFarRight;
    LE<i16> anmExLeft;
    LE<i16> anmExRight;
};

struct EclRawInstrSetInterruptArgs
{
    LE<i32> interruptSub;
    LE<i32> interruptId;
};

struct EclRawInstrSpellcardStartArgs
{
    LE<i16> spellcardSprite;
    LE<i16> spellcardId;
    char spellcardName[1];
};

struct EclRawInstrEffectParticleArgs
{
    LE<i32> effectId;
    LE<i32> numParticles;
    LE<ZunColor> particleColor;
};

struct EclRawInstrTimeSetArgs
{
    LE<EclVarId> timeToSet;
};

struct EclRawInstrDropItemArgs
{
    LE<ItemType> itemId;
};

struct EclRawInstrEnemyCreateArgs
{
    LE<i32> subId;
    ZunVec3Raw pos;
    LE<i16> life;
    LE<i16> itemDrop;
    LE<i32> score;
};

struct EclRawInstrAnmInterruptSlotArgs
{
    LE<i32> vmId;
    LE<i32> interruptId;
};

struct EclRawInstrBulletSoundArgs
{
    LE<SoundIdx> bulletSfx;
};

struct EclRawInstrBulletRankInfluenceArgs
{
    LE<f32> bulletRankSpeedLow;
    LE<f32> bulletRankSpeedHigh;
    LE<i32> bulletRankAmount1Low;
    LE<i32> bulletRankAmount1High;
    LE<i32> bulletRankAmount2Low;
    LE<i32> bulletRankAmount2High;
};

struct EclRawInstrExInstrArgs
{
    LE<u32> exInstrIndex;
    union {
        LE<i32> i32Param;
        u8 u8Param;
    };
};

union EclRawInstrArgs {
    EclRawInstrAluArgs alu;
    EclRawInstrCmpArgs cmp;
    EclRawInstrJumpArgs jump;
    EclRawInstrCallArgs call;
    EclRawInstrAnmSetMainArgs anmSetMain;
    EclRawInstrAnmSetPosesArgs anmSetPoses;
    EclRawInstrAnmSetSlotArgs anmSetSlot;
    EclRawInstrAnmSetDeathArgs anmSetDeath;
    EclRawInstrMoveArgs move;
    EclRawInstrBulletArgs bullet;
    EclRawInstrLaserArgs laser;
    EclRawInstrLaserOpArgs laserOp;
    EclRawInstrBulletEffectsArgs bulletEffects;
    EclRawInstrSpellcardEffectArgs spellcardEffect;
    EclRawInstrMoveBoundSetArgs moveBoundSet;
    EclRawInstrSetInterruptArgs setInterrupt;
    EclRawInstrSpellcardStartArgs spellcardStart;
    EclRawInstrEffectParticleArgs effectParticle;
    EclRawInstrTimeSetArgs timeSet;
    EclRawInstrDropItemArgs dropItem;
    EclRawInstrEnemyCreateArgs enemyCreate;
    EclRawInstrAnmInterruptSlotArgs anmInterruptSlot;
    EclRawInstrBulletSoundArgs bulletSound;
    EclRawInstrBulletRankInfluenceArgs bulletRankInfluence;
    EclRawInstrExInstrArgs exInstr;
    LE<i32> setInt;

    i32 GetBossLifeCount() const
    {
        return this->setInt;
    }
};

struct EclRawInstr
{
    LE<i32> time;
    LE<i16> opCode;
    LE<i16> offsetToNext;
    u8 unk_8;

    u8 skipForDifficulty;
    u8 unk_a;
    u8 unk_b;
    EclRawInstrArgs args;
};

struct EclRawHeader
{
    LE<i16> subCount;
    LE<i16> mainCount;
    LE<u32> timelineOffsets[3];
    LE<u32> subOffsets[0];
};

enum EclRawInstrOpcode
{
    ECL_OPCODE_NOP,
    ECL_OPCODE_UNIMP,
    ECL_OPCODE_JUMP,
    ECL_OPCODE_JUMPDEC,
    ECL_OPCODE_SETINT,
    ECL_OPCODE_SETFLOAT,
    ECL_OPCODE_SETINTRAND,
    ECL_OPCODE_SETINTRANDMIN,
    ECL_OPCODE_SETFLOATRAND,
    ECL_OPCODE_SETFLOATRANDMIN,
    ECL_OPCODE_SETVARSELFX,
    ECL_OPCODE_SETVARSELFY,
    ECL_OPCODE_SETVARSELFZ,
    ECL_OPCODE_MATHINTADD,
    ECL_OPCODE_MATHINTSUB,
    ECL_OPCODE_MATHINTMUL,
    ECL_OPCODE_MATHINTDIV,
    ECL_OPCODE_MATHINTMOD,
    ECL_OPCODE_MATHINC,
    ECL_OPCODE_MATHDEC,
    ECL_OPCODE_MATHFLOATADD,
    ECL_OPCODE_MATHFLOATSUB,
    ECL_OPCODE_MATHFLOATMUL,
    ECL_OPCODE_MATHFLOATDIV,
    ECL_OPCODE_MATHFLOATMOD,
    ECL_OPCODE_MATHATAN2,
    ECL_OPCODE_MATHNORMANGLE,
    ECL_OPCODE_CMPINT,
    ECL_OPCODE_CMPFLOAT,
    ECL_OPCODE_JUMPLSS,
    ECL_OPCODE_JUMPLEQ,
    ECL_OPCODE_JUMPEQU,
    ECL_OPCODE_JUMPGRE,
    ECL_OPCODE_JUMPGEQ,
    ECL_OPCODE_JUMPNEQ,
    ECL_OPCODE_CALL,
    ECL_OPCODE_RET,
    ECL_OPCODE_CALLLSS,
    ECL_OPCODE_CALLLEQ,
    ECL_OPCODE_CALLEQU,
    ECL_OPCODE_CALLGRE,
    ECL_OPCODE_CALLGEQ,
    ECL_OPCODE_CALLNEQ,
    ECL_OPCODE_MOVEPOSITION,
    ECL_OPCODE_MOVEAXISVELOCITY,
    ECL_OPCODE_MOVEVELOCITY,
    ECL_OPCODE_MOVEANGULARVELOCITY,
    ECL_OPCODE_MOVESPEED,
    ECL_OPCODE_MOVEACCELERATION,
    ECL_OPCODE_MOVERAND,
    ECL_OPCODE_MOVERANDINBOUND,
    ECL_OPCODE_MOVEATPLAYER,
    ECL_OPCODE_MOVEDIRTIMEDECELERATE,
    ECL_OPCODE_MOVEDIRTIMEDECELERATEFAST,
    ECL_OPCODE_MOVEDIRTIMEACCELERATE,
    ECL_OPCODE_MOVEDIRTIMEACCELERATEFAST,
    ECL_OPCODE_MOVEPOSITIONTIMELINEAR,
    ECL_OPCODE_MOVEPOSITIONTIMEDECELERATE,
    ECL_OPCODE_MOVEPOSITIONTIMEDECELERATEFAST,
    ECL_OPCODE_MOVEPOSITIONTIMEACCELERATE,
    ECL_OPCODE_MOVEPOSITIONTIMEACCELERATEFAST,
    ECL_OPCODE_MOVETIMEDECELERATE,
    ECL_OPCODE_MOVETIMEDECELERATEFAST,
    ECL_OPCODE_MOVETIMEACCELERATE,
    ECL_OPCODE_MOVETIMEACCELERATEFAST,
    ECL_OPCODE_MOVEBOUNDSSET,
    ECL_OPCODE_MOVEBOUNDSDISABLE,
    ECL_OPCODE_BULLETFANAIMED,
    ECL_OPCODE_BULLETFAN,
    ECL_OPCODE_BULLETCIRCLEAIMED,
    ECL_OPCODE_BULLETCIRCLE,
    ECL_OPCODE_BULLETOFFSETCIRCLEAIMED,
    ECL_OPCODE_BULLETOFFSETCIRCLE,
    ECL_OPCODE_BULLETRANDOMANGLE,
    ECL_OPCODE_BULLETRANDOMSPEED,
    ECL_OPCODE_BULLETRANDOM,
    ECL_OPCODE_SHOOTINTERVAL,
    ECL_OPCODE_SHOOTINTERVALDELAYED,
    ECL_OPCODE_SHOOTDISABLED,
    ECL_OPCODE_SHOOTENABLED,
    ECL_OPCODE_SHOOTNOW,
    ECL_OPCODE_SHOOTOFFSET,
    ECL_OPCODE_BULLETEFFECTS,
    ECL_OPCODE_BULLETCANCEL,
    ECL_OPCODE_BULLETSOUND,
    ECL_OPCODE_LASERCREATE,
    ECL_OPCODE_LASERCREATEAIMED,
    ECL_OPCODE_LASERINDEX,
    ECL_OPCODE_LASERROTATE,
    ECL_OPCODE_LASERROTATEFROMPLAYER,
    ECL_OPCODE_LASEROFFSET,
    ECL_OPCODE_LASERTEST,
    ECL_OPCODE_LASERCANCEL,
    ECL_OPCODE_SPELLCARDSTART,
    ECL_OPCODE_SPELLCARDEND,
    ECL_OPCODE_ENEMYCREATE,
    ECL_OPCODE_ENEMYKILLALL,
    ECL_OPCODE_ANMSETMAIN,
    ECL_OPCODE_ANMSETPOSES,
    ECL_OPCODE_ANMSETSLOT,
    ECL_OPCODE_ANMSETDEATH,
    ECL_OPCODE_BOSSSET,
    ECL_OPCODE_SPELLCARDEFFECT,
    ECL_OPCODE_ENEMYSETHITBOX,
    ECL_OPCODE_ENEMYFLAGCOLLISION,
    ECL_OPCODE_ENEMYFLAGCANTAKEDAMAGE,
    ECL_OPCODE_EFFECTSOUND,
    ECL_OPCODE_ENEMYFLAGDEATH,
    ECL_OPCODE_DEATHCALLBACKSUB,
    ECL_OPCODE_ENEMYINTERRUPTSET,
    ECL_OPCODE_ENEMYINTERRUPT,
    ECL_OPCODE_ENEMYLIFESET,
    ECL_OPCODE_BOSSTIMERSET,
    ECL_OPCODE_LIFECALLBACKTHRESHOLD,
    ECL_OPCODE_LIFECALLBACKSUB,
    ECL_OPCODE_TIMERCALLBACKTHRESHOLD,
    ECL_OPCODE_TIMERCALLBACKSUB,
    ECL_OPCODE_ENEMYFLAGINTERACTABLE,
    ECL_OPCODE_EFFECTPARTICLE,
    ECL_OPCODE_DROPITEMS,
    ECL_OPCODE_ANMFLAGROTATION,
    ECL_OPCODE_EXINSCALL,
    ECL_OPCODE_EXINSREPEAT,
    ECL_OPCODE_TIMESET,
    ECL_OPCODE_DROPITEMID,
    ECL_OPCODE_STDUNPAUSE,
    ECL_OPCODE_BOSSSETLIFECOUNT,
    ECL_OPCODE_DEBUGWATCH,
    ECL_OPCODE_ANMINTERRUPTMAIN,
    ECL_OPCODE_ANMINTERRUPTSLOT,
    ECL_OPCODE_ENEMYFLAGDISABLECALLSTACK,
    ECL_OPCODE_BULLETRANKINFLUENCE,
    ECL_OPCODE_ENEMYFLAGINVISIBLE,
    ECL_OPCODE_BOSSTIMERCLEAR,
    ECL_OPCODE_LASERCLEARALL,
    ECL_OPCODE_SPELLCARDFLAGTIMEOUT,
};

struct EclManager
{
    ZunResult Load(const char *ecl);
    void Unload();
    ZunResult RunEcl(Enemy *enemy);
    ZunResult CallEclSub(EnemyEclContext *enemyEcl, i16 subId) const;

    const EclRawHeader *eclFile;
    const EclTimelineInstr *timelinePtrs[3];
    EclRawInstr **subTable;
    const EclTimelineInstr *timeline;
};

extern EclManager g_EclManager;
