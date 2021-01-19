#pragma once
#include "pch.h"

using namespace cocos2d;

namespace PlayLayer {
	void setup(uintptr_t base);
	void unload(uintptr_t base);
	
	constexpr int StatusLabelTag = 666;
	inline CCLayer* self;
	inline uintptr_t base;

	inline void(__thiscall* init)(CCLayer* self, void*);
	void __fastcall initHook(CCLayer* self, void*, void*);

	void updateStatusLabel(const char* text);

	inline void(__thiscall* update)(CCLayer* self, float dt);
	void __fastcall updateHook(CCLayer* self, void*, float dt);

	inline void*(__thiscall* levelComplete)(CCLayer* self);
	void* __fastcall levelCompleteHook(CCLayer* self, void*);

	inline void* (__thiscall* onQuit)(CCLayer* self);
	void* __fastcall onQuitHook(CCLayer* self, void*);

	inline uint32_t(__thiscall* pushButton)(CCLayer* self, int, bool);
	uint32_t __fastcall pushButtonHook(CCLayer* self, void*, int, bool);

	inline uint32_t(__thiscall* releaseButton)(CCLayer* self, int, bool);
	uint32_t __fastcall releaseButtonHook(CCLayer* self, void*, int, bool);

	bool is2Player();

	uintptr_t getPlayer();
	uintptr_t getPlayer2();
};

