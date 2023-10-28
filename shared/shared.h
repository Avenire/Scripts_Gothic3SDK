#ifndef SCRIPTS_GOTHIC3SDK_SHARED
#define SCRIPTS_GOTHIC3SDK_SHARED


#include "util\Hook.h"

GELPVoid GE_STDCALL Call_mCBaseHook_GetOriginalFunction_GELPVOID(mCBaseHook* const hook);

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

#endif