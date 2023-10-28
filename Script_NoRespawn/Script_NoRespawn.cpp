#include "Script_NoRespawn.h"

#include "util/Logging.h"
#include "util/Hook.h"
#include "util/ScriptUtil.h"
#include "../shared/shared.h"
#include "Script.h"

gSScriptInit & GetScriptInit()
{
    static gSScriptInit s_ScriptInit;
    return s_ScriptInit;
}

// TODO: Add an option to allow monster respawn but without them giving XP/Loot?
// TODO: Add an optional notification if this happens?
GEInt GE_STDCALL RespawnHook(gFScript const a_orignalFunc, gCScriptProcessingUnit* a_pSPU, GELPVoid a_pSelfEntity, GELPVoid a_pOtherEntity, GEInt a_iArgs)
{
	INIT_SCRIPT();
	// Call the original "Respawn" script. It does more than just "respawning" i.e. it also makes sure bodies of
	// NPCs holding quest items don't get deleted from the game.
	GEInt Result = a_orignalFunc(a_pSPU, a_pSelfEntity, a_pOtherEntity, a_iArgs);
	if (Result 
		// Not sure what that was for so commenting out for now.
		// || (SelfEntity.NPC.Species == gESpecies::gESpecies_EMPTY_B && SelfEntity.Routine.AIMode == gEAIMode::gEAIMode_Dead)
	) {
		// Note: Not to be confused with "Kill" script.
		// This seems to be permanently deleting Entity from the game's world.
		SelfEntity.Kill();
	}
	return 0;
}

extern "C" __declspec( dllexport )
gSScriptInit const * GE_STDCALL ScriptInit( void )
{
	GetScriptAdmin().LoadScriptDLL("Script_Game.dll");
	auto& admin = GetScriptAdminExt();
	auto respawnScript = admin.GetScript("Respawn");
	Hook(respawnScript->m_funcScript, RespawnHook);
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