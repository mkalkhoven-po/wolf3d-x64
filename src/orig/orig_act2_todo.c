/*
 * Think/action functions referenced by the generated state tables that are
 * not ported yet: bosses with special attacks, BJ's victory run and the death
 * cam. They follow in P4/S2b. Each stops loudly instead of silently doing
 * nothing, so a missing port can't pass for original behaviour.
 */
#include "orig.h"

#include "id_us.h"

#define NOT_PORTED(name)                                                       \
    void name(objtype *ob)                                                     \
    {                                                                          \
        (void)ob;                                                              \
        Quit(#name ": not ported yet (P4/S2b)");                               \
    }

NOT_PORTED(A_HitlerMorph)
NOT_PORTED(A_MechaSound)
NOT_PORTED(A_Slurpie)
NOT_PORTED(A_StartDeathCam)
NOT_PORTED(T_BJDone)
NOT_PORTED(T_BJJump)
NOT_PORTED(T_BJRun)
NOT_PORTED(T_BJYell)
NOT_PORTED(T_Fake)
NOT_PORTED(T_FakeFire)
NOT_PORTED(T_Fat)
NOT_PORTED(T_Gift)
NOT_PORTED(T_GiftThrow)
NOT_PORTED(T_Schabb)
NOT_PORTED(T_SchabbThrow)
