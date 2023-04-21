#include "Script_NoRespawn.h"

#include "util/Logging.h"
#include "util/Hook.h"
#include "util/ScriptUtil.h"

#include "Script.h"

gSScriptInit & GetScriptInit()
{
    static gSScriptInit s_ScriptInit;
    return s_ScriptInit;
}

GELPVoid GE_STDCALL Call_mCBaseHook_GetOriginalFunction_GELPVOID(mCBaseHook* const hook) {
	return hook->GetOriginalFunction<GELPVoid>();
}

template<typename ReturnType, typename... ArgTypes>
using function_pointer = ReturnType(*const) (ArgTypes...);

template<typename ReturnType, typename... ArgTypes>
mCBaseHook* Hook(
	function_pointer<ReturnType, ArgTypes...> a_pOriginalFunc,
	function_pointer<ReturnType, function_pointer<ReturnType, ArgTypes...>, ArgTypes...> a_pWrapperFunc
) {
	using namespace asmjit;
	using namespace asmjit::x86;
	auto hook = GE_NEW(mCFunctionHook);

	X86CodeAsm Asm;
	//// EAX is caller-saved so we should preserve it.
	//// esp-4 will move result of Call_mCBaseHook_GetOriginalFunction_GELPVOID here
	//// esp-8 will use for Call_mCBaseHook_GetOriginalFunction_GELPVOID call arg and then move caller's return address here
	//// esp-12 unused here so should be safe to use that memory temporarily
	Asm.mov(dword_ptr(esp, -12), eax); 

	//// Take caller's return address and push it 4 bytes down the stack to make space
	//// for an extra first arg in wrapper functions.
	Asm.mov(eax, dword_ptr(esp));
	Asm.push(eax); // current_esp = original_esp-4

	//// Push hook pointer as first arg, call wrapper function and push new address of original func pointer from EAX 
	//// to the stack (in place where caller's return address used to be).
	//// Note: We CAN'T pass a_pOriginalFunc here because after hooking this will not point at the valid address of original function anymore.
	//// Note: Casting template method pointer to GEPLVoid is suprisingly cumbersome in C++. So instead of making direct method call here I call a regular function
	//// pointer that does that for me.
	Asm.push(imm_ptr(hook)); // current_esp = original_esp-8
	Asm.call(imm_ptr(Call_mCBaseHook_GetOriginalFunction_GELPVOID)); 
	//// stdcall convention makes the callee remove their args from ESP.
	Asm.mov(dword_ptr(esp, 4), eax); // current_esp = original_esp - 4
	//
	//// Restore EAX and jump to the start of the wrapper function.
	Asm.mov(eax, dword_ptr(esp, -8)); // original_esp - 12 (EAX) = current_esp (original_esp - 4) - 8
	Asm.jmp(imm_ptr((GELPVoid)a_pWrapperFunc));

	hook->Hook(a_pOriginalFunc, JitRuntimeAdd(Asm), mCBaseHook::mEHookType_OnlyStack);
	return hook;
}

GEInt GE_STDCALL RespawnHook(gFScript const a_orignalFunc, gCScriptProcessingUnit* a_pSPU, GELPVoid a_pSelfEntity, GELPVoid a_pOtherEntity, GEInt a_iArgs)
{
	INIT_SCRIPT();
	// Call original "Respawn" script because it does plenty useful checks (entity contains quest items, entity was "dead" less than 5/10 in game days etc).
	GEInt Result = a_orignalFunc(a_pSPU, a_pSelfEntity, a_pOtherEntity, a_iArgs);
	if (Result || (SelfEntity.NPC.Species == gESpecies::gESpecies_EMPTY_B && SelfEntity.Routine.AIMode == gEAIMode::gEAIMode_Dead)) {
		// Note: Kill method doesn't trigger Kill script, it deletes entity from the game.
		SelfEntity.Kill();
	}
	return 0;
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

GEInt GE_STDCALL KillHook(gFScript const a_orignalFunc, gCScriptProcessingUnit* a_pSPU, GELPVoid a_pSelfEntity, GELPVoid a_pOtherEntity, GEInt a_iArgs)
{
	INIT_SCRIPT();
	auto& victimRoutinePropSet = OtherEntity.Routine;
	auto& victimNPCPropSet = OtherEntity.NPC;
	auto& victimEnclavePropSet = OtherEntity.Enclave;
	// We must observe the value BEFORE calling original Kill script because this flag to true if NPC kills other NPC...
	auto wasDefeatedByPlayer = victimNPCPropSet.DefeatedByPlayer;

	// Call original Kill script (or whatever was registered there at the time of hooking...)
	GEInt Result = a_orignalFunc(a_pSPU, a_pSelfEntity, a_pOtherEntity, a_iArgs);

	// Any cases where kill is not either of:
	auto player = Entity::GetPlayer();
	auto killerValid = SelfEntity.GetInstance() && SelfEntity.GetInstance()->IsValid();
	
	auto wasExperienceStolen = !(
		// Skip notification if:
		// Kill script was called but apparently there was no call (shouldn't ever happen... right?)
		!Result ||
		// Player killed
		SelfEntity.IsPlayer() ||
		// Player was killed
		OtherEntity.IsPlayer() ||
		(killerValid && (
			// Killed entity was already defeated before and XP granted
			wasDefeatedByPlayer ||
			// Player's team-mate killed
			IsInSamePartyAsPlayer(SelfEntity) ||
			// Summoned create was killed
			OtherEntity.Party.PartyMemberType == ::gEPartyMemberType_Summoned
		)) ||
		// Fleeing human/orc and likely got killed by the game engine once they fled out of processing range.
		!killerValid &&
		victimRoutinePropSet.AIMode == ::gEAIMode_Flee &&
		victimEnclavePropSet.Status == ::gEEnclaveStatus_Flee &&
		(victimNPCPropSet.Species == gESpecies::gESpecies_Human || victimNPCPropSet.Species == gESpecies::gESpecies_Orc)
		// Know "issue" - Kills via kill cheat will count as "stolen" even though hero gets XP
	);
	if (wasExperienceStolen) {
		// If killer name is not valid (NPC dies from fall, drowining, cheat kill etc... perhaps there's more edge cases)
		// assume "Beliar" killed them and stole our XP ;) Otherwise use entity's Focus Name.
		// Using Unicode ⚔/⚡ as "kills" because I can't be bothered with custom string tables and localizing.
		auto killerName = killerValid ? 
			bCUnicodeString::GetFormattedString(L"%s ⚔", SelfEntity.GetFocusName()) :
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
	auto& admin = GetScriptAdminExt();
	auto respawnScript = admin.GetScript("Respawn");
	Hook(respawnScript->m_funcScript, RespawnHook);
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