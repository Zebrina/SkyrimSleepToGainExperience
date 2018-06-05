#include "SKSEMemUtil.h"

#include "skse64_common/SafeWrite.h"

void __stdcall SKSEMemUtil::WriteRedirectionHook(IntPtr targetOfHook, IntPtr sourceBegin, IntPtr sourceEnd, UInt32 asmSegSize) {
	SafeWriteJump(targetOfHook, sourceBegin);

	for (int i = 5; i < asmSegSize; ++i) {
		SafeWrite8(targetOfHook + i, 0x90); // nop
	}

	SafeWriteJump(sourceEnd, targetOfHook + asmSegSize);
}
SKSEMemUtil::IntPtr __stdcall SKSEMemUtil::WriteVTableHook(IntPtr vtable, UInt32 fn, IntPtr fnPtr) {
	IntPtr originalFunction = vtable[fn];

	SafeWrite64(vtable + fn * sizeof(void*), fnPtr);

	return originalFunction;
}

void __stdcall SKSEMemUtil::WriteOpCode8(IntPtr target, OpCode8 op) {
	SafeWrite8(target, op);
}
void __stdcall SKSEMemUtil::WriteOpCode16(IntPtr target, OpCode16 op) {
	SafeWrite16(target, op);
}
void __stdcall SKSEMemUtil::WriteOpCode24(IntPtr target, OpCode24 op) {
	SafeWriteBuf(target, reinterpret_cast<void*>(&op), 3);
}
void __stdcall SKSEMemUtil::WriteOpCode8WithImmediate(IntPtr target, OpCode8 op, IntPtr var) {
	SafeWrite8(target, op);
	SafeWrite64(target + 0x1, var);
}
void __stdcall SKSEMemUtil::WriteOpCode16WithImmediate(IntPtr target, OpCode16 op, IntPtr var) {
	SafeWrite16(target, op);
	SafeWrite64(target + 0x2, var);
}
void __stdcall SKSEMemUtil::WriteOpCode24WithImmediate(IntPtr target, OpCode24 op, IntPtr var) {
	SafeWriteBuf(target, reinterpret_cast<void*>(&op), 3);
	SafeWrite64(target + 0x3, var);
}
