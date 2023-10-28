#include "shared.h"
#include "util/Logging.h"
#include "util/Hook.h"
#include "util/ScriptUtil.h"

GELPVoid GE_STDCALL Call_mCBaseHook_GetOriginalFunction_GELPVOID(mCBaseHook* const hook) {
	return hook->GetOriginalFunction<GELPVoid>();
}