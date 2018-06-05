#pragma once

#include <utility>

STATIC_ASSERT(sizeof(void*) == sizeof(UInt64));

#define _SetFlags(flags, mask) ((flags)|=(mask))
#define _ClearFlags(flags, mask) ((flags)&=(~(mask)))
#define _TestFlags(flags, mask) (((flags)&(mask))!=0)

#define _GetObjectVTable(ptr) SKSEMemUtil::IntPtr(*(UInt64**)(ptr))
#define _CallMemberFunction(thisPtr, method) ((thisPtr)->*(method))

#define _AsmBasicHookPrefix(id, addr, retnOffset)	\
		__asm push dword ptr retnOffset				\
		__asm push _ASM_##id##_END_OF_HOOK			\
		__asm push _ASM_##id##_START_OF_HOOK		\
		__asm push addr								\
		__asm call SKSEMemUtil::WriteRedirectionHook	\
		__asm jmp _ASM_##id##_SKIP_HOOK				\
	__asm _ASM_##id##_START_OF_HOOK:

#define _AsmRedirectHookPrefix(id, addr, retnOffset, newFunc, oldFuncPtr)	\
		__asm push eax														\
		__asm mov eax, _ASM_##id##_ORIGINAL_CODE							\
		__asm mov oldFuncPtr, eax											\
		__asm pop eax														\
		__asm push dword ptr retnOffset										\
		__asm push _ASM_##id##_END_OF_HOOK									\
		__asm push newFunc													\
		__asm push addr														\
		__asm call SKSEMemUtil::WriteRedirectionHook							\
		__asm jmp _ASM_##id##_SKIP_HOOK										\
	__asm _ASM_##id##_ORIGINAL_CODE:

#define _AsmHookSuffix(id)					\
	__asm _ASM_##id##_END_OF_HOOK:			\
		__asm nop							\
		__asm nop							\
		__asm nop							\
		__asm nop							\
		__asm nop							\
	__asm _ASM_##id##_SKIP_HOOK:

namespace SKSEMemUtil {
	class IntPtr {
	public:
		template <typename T>
		IntPtr(const T type) :
			data(castFrom<T>(type)) {
		}
		IntPtr(const IntPtr& other) :
			data(other.data) {
		}

		template <typename T>
		IntPtr& operator=(const T value) {
			data = castFrom<T>(value);
			return *this;
		}

		template <typename T>
		operator T() const {
			return castTo<T>(data);
		}

		template <typename T>
		bool operator==(const T other) const {
			return data == (UInt64)other;
		}
		template <typename T>
		bool operator!=(const T other) const {
			return data != (UInt64)other;
		}

		template <typename T>
		bool operator<(const T other) const {
			return data < (UInt64)other;
		}
		template <typename T>
		bool operator<=(const T other) const {
			return data <= (UInt64)other;
		}
		template <typename T>
		bool operator>(const T other) const {
			return data > (UInt64)other;
		}
		template <typename T>
		bool operator>=(const T other) const {
			return data >= (UInt64)other;
		}

		template <typename T>
		IntPtr operator+(const T value) const {
			return IntPtr(data + castFrom<T>(value));
		}
		template <typename T>
		IntPtr& operator+=(const T value) {
			data += castFrom<T>(value);
			return *this;
		}

		template <typename T>
		IntPtr operator-(const T value) const {
			return IntPtr(data - castFrom<T>(value));
		}
		template <typename T>
		IntPtr& operator-=(const T value) {
			data -= castFrom<T>(value);
			return *this;
		}

		template <typename T>
		IntPtr operator*(const T value) const {
			return IntPtr(data * castFrom<T>(value));
		}
		template <typename T>
		IntPtr& operator*=(const T value) {
			data *= castFrom<T>(value);
			return *this;
		}

		template <typename T>
		IntPtr operator/(const T value) const {
			return IntPtr(data / castFrom<T>(value));
		}
		template <typename T>
		IntPtr& operator/=(const T value) {
			data /= castFrom<T>(value);
			return *this;
		}

		template <typename T>
		IntPtr operator%(const T value) const {
			return IntPtr(data + castFrom<T>(value));
		}
		template <typename T>
		IntPtr& operator%=(const T value) {
			data %= castFrom<T>(value);
			return *this;
		}

		const IntPtr& operator[](int index) const {
			return *((IntPtr*)(data + index * sizeof(IntPtr)));
		}

	private:
		UInt64 data;

		template <typename T>
		union Castable {
			UInt64 data;
			T type;
		};

		template <typename T>
		static T castTo(UInt64 data) {
			Castable<T> c;
			c.data = data;
			return c.type;
		}
		template <typename T>
		static UInt64 castFrom(T type) {
			Castable<T> c;
			c.type = type;
			return c.data;
		}
	};

	enum OpCode8 : UInt8 {
		Mov_BytePtrImm_AL = 0xa2,

		Jb_Rel = 0x72,
		Jna_Rel = 0x76,
		Ja_Rel = 0x77,
	};
	enum OpCode16 : UInt16 {
		Cmp_BytePtrImm_AL = 0x0538,
		Cmp_BytePtrImm_BL = 0x1d38,
		Cmp_BytePtrImm_CL = 0x0d38,
		Cmp_BytePtrImm_DL = 0x1538,
		Cmp_BytePtrImm_AH = 0x2538,
		Cmp_BytePtrImm_BH = 0x3d38,
		Cmp_BytePtrImm_CH = 0x2d38,
		Cmp_BytePtrImm_DH = 0x3538,
		Cmp_BytePtrImm_Imm = 0x3d80,

		Cmp_DwordPtrImm_EAX = 0x0539,
		Cmp_DwordPtrImm_EBX = 0x1d39,
		Cmp_DwordPtrImm_ECX = 0x0d39,
		Cmp_DwordPtrImm_EDX = 0x1539,
		Cmp_DwordPtrImm_ESI = 0x3539,
		Cmp_DwordPtrImm_EDI = 0x3d39,
		Cmp_DwordPtrImm_EBP = 0x2d39,
		Cmp_DwordPtrImm_ESP = 0x2539,
		Cmp_DwordPtrImm_Imm = 0x3d83,

		Mov_ECX_DwordPtrImm = 0x8e8b,
	};
	enum OpCode24 : UInt32 {
		Movzx_BytePtrImm = 0x0db60f,
	};

	void __stdcall WriteRedirectionHook(IntPtr targetOfHook, IntPtr sourceBegin, IntPtr sourceEnd, UInt32 asmSegSize);
	IntPtr __stdcall WriteVTableHook(IntPtr vtable, UInt32 fn, IntPtr fnPtr);

	void __stdcall WriteOpCode8(IntPtr target, OpCode8 op);
	void __stdcall WriteOpCode16(IntPtr target, OpCode16 op);
	void __stdcall WriteOpCode24(IntPtr target, OpCode24 op);
	void __stdcall WriteOpCode8WithImmediate(IntPtr target, OpCode8 op, IntPtr var);
	void __stdcall WriteOpCode16WithImmediate(IntPtr target, OpCode16 op, IntPtr var);
	void __stdcall WriteOpCode24WithImmediate(IntPtr target, OpCode24 op, IntPtr var);
}

template <>
struct std::hash<SKSEMemUtil::IntPtr> {
	size_t operator()(SKSEMemUtil::IntPtr iptr) const {
		return iptr;
	}
};

STATIC_ASSERT(sizeof(SKSEMemUtil::IntPtr) == sizeof(UInt64));
