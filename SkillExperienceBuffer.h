#pragma once

#define SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL

class Actor;

typedef void(*Actor_AdvanceSkill_Method)(Actor*, UInt32, float, UInt32, UInt32);
extern Actor_AdvanceSkill_Method PlayerCharacter_AdvanceSkill;

struct SkillExperienceBuffer {
	enum : UInt32 {
		OneHanded = 6,
		FirstSkillId = OneHanded,
		TwoHanded,
		Marksman,		// Archery
		Block,
		Smithing,
		HeavyArmor,
		LightArmor,
		Pickpocket,
		LockPicking,
		Sneak,
		Alchemy,
		SpeechCraft,	// Speech
		Alteration,
		Conjuration,
		Destruction,
		Mysticism,		// Illusion
		Restoration,
		Enchanting,
		LastSkillId = Enchanting,
		SkillCount = (LastSkillId - FirstSkillId) + 1,
	};

	float expBuf[SkillCount];
#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
	float flushedExpBuf[SkillCount];
	UInt16 skillStartLevel[SkillCount];
#endif

	SkillExperienceBuffer() {
		ZeroMemory(this, sizeof(SkillExperienceBuffer));
	}

	float getExperience(UInt32 skillId) const;

	void addExperience(UInt32 skillId, float points);

	void flushExperience(float percent);
	void flushExperienceBySkill(UInt32 skillId, float percent);

	void multExperience(float mult);
	void multExperienceBySkill(UInt32 skillId, float mult);

	void clear();
	void clearBySkill(UInt32 skillId);
};