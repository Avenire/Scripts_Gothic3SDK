#include "Script_GotoXP.h"

#include "util/Logging.h"
#include "util/Hook.h"
#include "util/ScriptUtil.h"
#include "../shared/shared.hpp"
#include "Script.h"
#include <util/Util.h>
#include <util/Memory.h>

gSScriptInit & GetScriptInit()
{
    static gSScriptInit s_ScriptInit;
    return s_ScriptInit;
}
static mCFunctionHook HookOnExecute;
GEBool OnExecuteHook(bCString const& a_rCommand, bCString& a_rStrResult)
{
	bTObjArray< bCString > arrParams = SplitString(a_rCommand, " ", GEFalse, GEFalse);
	
	GEInt iParamCount = arrParams.GetCount();
	Entity Player = Entity::GetPlayer();

	if (iParamCount > 0 && arrParams[0].CompareNoCaseFast(bCString("gotoxp")))
	{
		eCEntity* found = nullptr;
		auto idx = 0;
		if (iParamCount > 1)
		{
			bCString Search = arrParams[1];
			idx = Search.GetInteger();
		}
		auto NPCs = Entity::GetEntities();
		auto entityMap = FindModule<eCSceneAdmin>()->m_mapEntities;

		auto foundCnt = 0;
		auto foundIdx = 0;
		for (auto Iter = entityMap.Begin(); Iter != entityMap.End(); Iter++)
		{
			if (*Iter == nullptr)
			{
				continue;
			}

			eCEntity& entity = *(*Iter);
			if (!entity.HasPropertySet(eEPropertySetType_NPC)) { continue; };
			{
				Entity target = Entity(*Iter);
				//if (!NPC.IsNPC()) { continue;  }
				if (!target.NPC.DefeatedByPlayer) {
					if (foundIdx < idx) {
						found = *Iter;
						foundIdx++;
					};
					foundCnt++;
				};
			}

		}
		if (found) {
			Entity target = Entity(found);
			Entity::GetPlayer().Teleport(target);

			a_rStrResult  = bCString::GetFormattedString("Teleported to NPC %d/%d", foundIdx, foundCnt);
			return GETrue;
		}
		else {
			a_rStrResult = "Could not find NPC";
		};
		return GEFalse;
	}
	if (iParamCount > 0 && arrParams[0].CompareNoCaseFast(bCString("gotopp")))
	{
		eCEntity* found = nullptr;
		auto idx = 0;
		if (iParamCount > 1)
		{
			bCString Search = arrParams[1];
			idx = Search.GetInteger();
		}
		auto NPCs = Entity::GetEntities();
		auto entityMap = FindModule<eCSceneAdmin>()->m_mapEntities;

		auto foundCnt = 0;
		auto foundIdx = 0;
		for (auto Iter = entityMap.Begin(); Iter != entityMap.End(); Iter++)
		{
			if (*Iter == nullptr)
			{
				continue;
			}

			eCEntity& entity = *(*Iter);
			if (!entity.HasPropertySet(eEPropertySetType_Dialog) || !entity.HasPropertySet(eEPropertySetType_Inventory)) { continue; };
			{
				Entity target = Entity(*Iter);
				//if (!NPC.IsNPC()) { continue;  }
				if (
					!target.Dialog.PickedPocket && 
					GetTreasureSet(target.Inventory, gETreasureDistribution::gETreasureDistribution_Pickpocket) != None
				) {
					if (foundIdx < idx) {
						found = *Iter;
						foundIdx++;
					};
					foundCnt++;
				};
			}

		}
		if (found) {
			Entity target = Entity(found);
			Entity::GetPlayer().Teleport(target);

			a_rStrResult = bCString::GetFormattedString("Teleported to NPC %d/%d", foundIdx, foundCnt);
			return GETrue;
		}
		else {
			a_rStrResult = "Could not find NPC";
		};
		return GEFalse;
	}

	return HookOnExecute.GetOriginalFunction(&OnExecuteHook)(a_rCommand, a_rStrResult);
}
extern "C" __declspec( dllexport )
gSScriptInit const * GE_STDCALL ScriptInit( void )
{
	GetScriptAdmin().LoadScriptDLL("Script_Game.dll");
	HookOnExecute.Hook(PROC_Engine("?OnExecute@eCConsole@@MAE_NABVbCString@@AAV2@@Z"), &OnExecuteHook, mCFunctionHook::mEHookType_ThisCall);
    return &GetScriptInit();
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID )
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        ::DisableThreadLibraryCalls( hModule );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}