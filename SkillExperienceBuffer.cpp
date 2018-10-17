#include "SkillExperienceBuffer.h"

#include "skse64/GameAPI.h"
#include "skse64/GameReferences.h"
#include "skse64/GameForms.h"
#include "skse64/GameData.h"
#include "skse64/PapyrusActorValueInfo.h"

using namespace papyrusActorValueInfo;

Actor_AdvanceSkill_Method PlayerCharacter_AdvanceSkill = nullptr;

#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
struct SkillPointsByLevel {
	UInt32 level = 0;
	float points = 0.0f;

	void update(UInt32 skillId, UInt32 newLevelCap) {
		ActorValueInfo* avi = ActorValueList::GetSingleton()->GetActorValue(skillId);
		while (level < newLevelCap) {
			points += GetExperienceForLevel(avi, ++level);
		}
	}
} g_skillPointsByLevel[SkillExperienceBuffer::SkillCount];
#endif

float SkillExperienceBuffer::getExperience(UInt32 skillId) const {
	return expBuf[skillId];
}

void SkillExperienceBuffer::addExperience(UInt32 skillId, float points) {
	expBuf[skillId - FirstSkillId] += points;
}

void SkillExperienceBuffer::flushExperience(float percent) {
	for (UInt32 skillId = FirstSkillId; skillId <= LastSkillId; ++skillId) {
		flushExperienceBySkill(skillId, percent);
	}
}
void SkillExperienceBuffer::flushExperienceBySkill(UInt32 skillId, float percent) {
	int skillIndex = skillId - FirstSkillId;
	PlayerCharacter* pPC = *g_thePlayer;

#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
	float skillLevel = pPC->actorValueOwner.GetBase(skillId);

	if (skillStartLevel[skillIndex] == 0) {
		// Initialize skill start level.
		skillStartLevel[skillIndex] = skillLevel;
	}

	UInt32 playerLevel = CALL_MEMBER_FN(pPC, GetLevel)();

	if (g_skillPointsByLevel[skillIndex].level < playerLevel) {
		if (g_skillPointsByLevel[skillIndex].level == 0) {
			g_skillPointsByLevel[skillIndex].level = skillLevel;
		}
		g_skillPointsByLevel[skillIndex].update(skillId, playerLevel);
	}
#endif

	float toAdd = expBuf[skillIndex] * percent;

	if (toAdd > 0.0f) {
#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
		flushedExpBuf[skillIndex] += toAdd;
		if (flushedExpBuf[skillIndex] > g_skillPointsByLevel[skillIndex].points) {
			toAdd -= (flushedExpBuf[skillIndex] - g_skillPointsByLevel[skillIndex].points);
			flushedExpBuf[skillIndex] = g_skillPointsByLevel[skillIndex].points;
		}
#endif

		PlayerCharacter_AdvanceSkill(pPC, skillId, toAdd, 0, 0);
		expBuf[skillIndex] -= toAdd;
	}
}

void SkillExperienceBuffer::multExperience(float mult) {
	for (UInt32 skillId = FirstSkillId; skillId <= LastSkillId; ++skillId) {
		multExperienceBySkill(skillId, mult);
	}
}
void SkillExperienceBuffer::multExperienceBySkill(UInt32 skillId, float mult) {
	expBuf[skillId - FirstSkillId] *= mult;
}

void SkillExperienceBuffer::clear() {
	ZeroMemory(expBuf, sizeof(expBuf));
}
void SkillExperienceBuffer::clearBySkill(UInt32 skillId) {
	expBuf[skillId - FirstSkillId] = 0.0f;
}