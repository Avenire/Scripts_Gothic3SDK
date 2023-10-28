#include "Script_DeathAlerts.h"

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

GEBool GE_STDCALL IsInSamePartyAsPlayer(Entity& npc) {
	auto& party = npc.Party;
	if (party.GetPartyLeader().IsPlayer()) {
		return true;
	}
	auto partyMembers = party.GetMembers(false);
	auto playerEngineEntity = Entity::GetPlayer().GetInstance();
	for (auto it = partyMembers.Begin(); it != partyMembers.End(); ++it) {
		if (*it == playerEngineEntity) {
			return true;
		}
	}
	return false;
}


// TODO: play an optional visual effect over the body until it was looted/looked at/went out of processing range or sth?
GEInt GE_STDCALL KillHook(gFScript const a_orignalFunc, gCScriptProcessingUnit* a_pSPU, GELPVoid a_pSelfEntity, GELPVoid a_pOtherEntity, GEInt a_iArgs)
{
	INIT_SCRIPT();
	auto& victimRoutinePropSet = OtherEntity.Routine;
	auto& victimNPCPropSet = OtherEntity.NPC;
	auto& victimEnclavePropSet = OtherEntity.Enclave;
	// We must save the value before  calling the original "Kill" script.
	// Game sets this flag as true no matter the actual killer.
	auto wasDefeatedByPlayer = victimNPCPropSet.DefeatedByPlayer;

	GEInt Result = a_orignalFunc(a_pSPU, a_pSelfEntity, a_pOtherEntity, a_iArgs);

	auto player = Entity::GetPlayer();
	// "Invalid" SelfEntity AKA the killer is set when NPC dies as a result of "kill" cheat, drowning, fall damage.
	// Could be there're more edge cases but this worked good enough so far.
	auto killerValid = SelfEntity.GetInstance() && SelfEntity.GetInstance()->IsValid();
	
	auto ripXP = !(
		// Listing all cases I could think of where player gets XP.
		// Case "Kill" script returns false.
		!Result ||
		// Player killed
		SelfEntity.IsPlayer() ||
		// Player was killed, lol
		OtherEntity.IsPlayer() ||
		(killerValid && (
			// Killed entity was already defeated before and XP granted
			wasDefeatedByPlayer ||
			// Player's team-mate killed
			IsInSamePartyAsPlayer(SelfEntity) ||
			// Summoned creature was killed
			OtherEntity.Party.PartyMemberType == ::gEPartyMemberType_Summoned
		)) ||
		// Fleeing human/orc, killer is not "valid" NPC and enclave's (city, camp etc) status is also "flee".
		// This means NPC got killed by the engine once they ran out of processing range.
		!killerValid &&
		victimRoutinePropSet.AIMode == ::gEAIMode_Flee &&
		victimEnclavePropSet.Status == ::gEEnclaveStatus_Flee &&
		(victimNPCPropSet.Species == gESpecies::gESpecies_Human || victimNPCPropSet.Species == gESpecies::gESpecies_Orc)
	);
	if (ripXP) {
		// Using Unicode ⚔/⚡ because I can't be bothered to get properly localized messages.
		auto killerName = killerValid ? 
			bCUnicodeString::GetFormattedString(L"%s ⚔", SelfEntity.GetFocusName()) :
			// Assume "Beliar" is behind "invalid" NPC kills because I don't feel like figuring out how to detect
			// all instances where it happens reliably.
			bCUnicodeString::GetFormattedString(L"%s ⚡", eCLocString("FO_Beliar_Tester").GetString());
		auto victimName = OtherEntity.GetFocusName();
		gCSession::GetSession().GetGUIManager()->PrintGameMessage(
			bCUnicodeString::GetFormattedString(L"%s %s (💀)❕", killerName, victimName),
			gEGameMessageType::gEGameMessageType_Failure
		);
	}
	return Result;
}

extern "C" __declspec( dllexport )
gSScriptInit const * GE_STDCALL ScriptInit( void )
{
	GetScriptAdmin().LoadScriptDLL("Script_Game.dll");
	auto& admin = GetScriptAdminExt();
	auto killScript = admin.GetScript("Kill");
	Hook(killScript->m_funcScript, KillHook);
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