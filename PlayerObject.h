#pragma once
#include "pch.h"

namespace PlayerObject {
	void setup(uintptr_t base);
	void unload(uintptr_t base);

	inline bool preventInput = false;

	inline void(__thiscall* pushButton)(void* self, void*);
	void __fastcall pushButtonHook(void* self, void*, void*);

	inline void(__thiscall* releaseButton)(void* self, void*);
	void __fastcall releaseButtonHook(void* self, void*, void*);

	float* getX(uintptr_t);
	float* getYAccel(uintptr_t);
	float* getRotation(uintptr_t);
	float* getSpriteRotation(uintptr_t);
}

