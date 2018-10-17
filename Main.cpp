#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64/GameAPI.h"

#include "skse64/GameData.h"
#include "skse64/GameReferences.h"
#include "skse64/GameEvents.h"
#include "skse64/GameTypes.h"
#include "skse64/GameMenus.h"

#include "skse64/PapyrusNativeFunctions.h"

#include "SKSEMemUtil.h"
#include "SkillExperienceBuffer.h"

#include <cmath>

constexpr char* PapyrusClassName = "SleepToGainExperience";

constexpr UInt32 pcVTableAdvanceSkillOffset = 0xF7;
//constexpr UInt32 pcVTableAdvanceSkillMemOffset = pcVTableAdvanceSkillOffset * 4;

IDebugLog gLog("SleepToGainExperience.log");

PluginHandle g_pluginHandle = kPluginHandle_Invalid;

SKSESerializationInterface* g_serialization = nullptr;
SKSEMessagingInterface*		g_messaging = nullptr;
SKSEPapyrusInterface*		g_papyrus = nullptr;

struct Settings {
	UInt32 enableSleepTimeRequirement : 1;
UInt32: 31;
	float percentExpRequiresSleep;
	float minDaysSleepNeeded;
	float interuptedPenaltyPercent;

	Settings() {
		ZeroMemory(this, sizeof(Settings));
	}
};

Settings g_settings;
SkillExperienceBuffer g_experienceBuffer;

void PlayerCharacter_AdvanceSkill_Hooked(Actor* _this, UInt32 skillId, float points, UInt32 unk1, UInt32 unk2) {
	static BSFixedString ConsoleMenu = "Console";
	static BSFixedString ConsoleNativeUIMenu = "Console Native UI Menu";

	MenuManager* menuManager = MenuManager::GetSingleton();
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId &&
		_this == *g_thePlayer &&
		menuManager && !menuManager->IsMenuOpen(&ConsoleMenu) && !menuManager->IsMenuOpen(&ConsoleNativeUIMenu)) {

		g_experienceBuffer.addExperience(skillId, g_settings.percentExpRequiresSleep * points);

		// call the original function but with points = 0
		// not sure what effect not calling it might have
		PlayerCharacter_AdvanceSkill(_this, skillId, (1.0f - g_settings.percentExpRequiresSleep) * points, unk1, unk2);
	}
	else {
		PlayerCharacter_AdvanceSkill(_this, skillId, points, unk1, unk2);
	}

	_MESSAGE("AdvanceSkill_Hooked end");
}

void Serialization_Revert(SKSESerializationInterface* serializationInterface)
{
	ZeroMemory(&g_experienceBuffer, sizeof(g_experienceBuffer));
}

constexpr UInt32 serializationDataVersion = 1;

void Serialization_Save(SKSESerializationInterface* serializationInterface) {
	_MESSAGE("Serialization_Save begin");

	if (serializationInterface->OpenRecord('DATA', serializationDataVersion)) {
		serializationInterface->WriteRecordData(&g_experienceBuffer, sizeof(g_experienceBuffer));
	}

	_MESSAGE("Serialization_Save end");
}

void Serialization_Load(SKSESerializationInterface* serializationInterface) {
	_MESSAGE("Serialization_Load begin");

	UInt32 type;
	UInt32 version;
	UInt32 length;
	bool error = false;

	while (!error && serializationInterface->GetNextRecordInfo(&type, &version, &length)) {
		if (type == 'DATA') {
			if (version == serializationDataVersion) {
				if (length == sizeof(g_experienceBuffer)) {
					serializationInterface->ReadRecordData(&g_experienceBuffer, length);

					_MESSAGE("read data");
				}
#ifndef SKILLEXPERIENCEBUFFER_NOEXPERIMENTAL
				else if (length == sizeof(g_experienceBuffer.expBuf)) {
					serializationInterface->ReadRecordData(&g_experienceBuffer, length);

					_MESSAGE("read data in old format");
				}
#endif
				else {
					_MESSAGE("empty or invalid data");
				}
			}
			else {
				error = true;
				_MESSAGE("version mismatch! read data version is %i, expected %i", version, serializationDataVersion);
			}
		}
		else {
			_MESSAGE("unhandled type %08X", type);
			error = true;
		}
	}

	_MESSAGE("Serialization_Load end");
}

void Messaging_Callback(SKSEMessagingInterface::Message* msg) {
	_MESSAGE("Messaging_Callback begin");

	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded) {
		// All forms are loaded

		_MESSAGE("kMessage_DataLoaded");

		if ((*g_dataHandler)->LookupLoadedModByName("SleepToGainExperience.esp")) {
			PlayerCharacter_AdvanceSkill = SKSEMemUtil::WriteVTableHook(_GetObjectVTable(*g_thePlayer), pcVTableAdvanceSkillOffset, PlayerCharacter_AdvanceSkill_Hooked);
		} else {
			_MESSAGE("game plugin not enabled");
		}
	}

	_MESSAGE("Messaging_Callback end");
}

float GetBufferedPointsBySkill(StaticFunctionTag*, BSFixedString skillName) {
	UInt32 skillId = LookupActorValueByName(skillName.data);
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId) {
		return g_experienceBuffer.getExperience(skillId);
	}
	return 0.0f;
}
void FlushBufferedExperience(StaticFunctionTag*, float daysSlept, bool interupted) {
	g_experienceBuffer.flushExperience((g_settings.enableSleepTimeRequirement ? fminf(1.0f, daysSlept / g_settings.minDaysSleepNeeded) : 1.0f) * (interupted ? g_settings.interuptedPenaltyPercent : 1.0f));
}
void FlushBufferedExperienceBySkill(StaticFunctionTag*, BSFixedString skillName, float daysSlept, bool interupted) {
	UInt32 skillId = LookupActorValueByName(skillName.data);
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId) {
		g_experienceBuffer.flushExperienceBySkill(skillId, (g_settings.enableSleepTimeRequirement ? fminf(1.0f, daysSlept / g_settings.minDaysSleepNeeded) : 1.0f) * (interupted ? g_settings.interuptedPenaltyPercent : 1.0f));
	}
}
void MultiplyBufferedExperience(StaticFunctionTag*, float value) {
	g_experienceBuffer.multExperience(value);
}
void MultiplyBufferedExperienceBySkill(StaticFunctionTag*, BSFixedString skillName, float value) {
	UInt32 skillId = LookupActorValueByName(skillName.data);
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId) {
		g_experienceBuffer.multExperienceBySkill(skillId, value);
	}
}
void ClearBufferedExperience(StaticFunctionTag*) {
	g_experienceBuffer.clear();
}
void ClearBufferedExperienceBySkill(StaticFunctionTag*, BSFixedString skillName) {
	UInt32 skillId = LookupActorValueByName(skillName.data);
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId) {
		g_experienceBuffer.clearBySkill(skillId);
	}
}

bool Papyrus_RegisterFunctions(VMClassRegistry* registry) {
	registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, float, BSFixedString>("GetBufferedPointsBySkill", PapyrusClassName, GetBufferedPointsBySkill, registry));
	registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, float, bool>("FlushBufferedExperience", PapyrusClassName, FlushBufferedExperience, registry));
	registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, BSFixedString, float, bool>("FlushBufferedExperienceBySkill", PapyrusClassName, FlushBufferedExperienceBySkill, registry));
	registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, float>("MultiplyBufferedExperience", PapyrusClassName, MultiplyBufferedExperience, registry));
	registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, float>("MultiplyBufferedExperienceBySkill", PapyrusClassName, MultiplyBufferedExperienceBySkill, registry));
	registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ClearBufferedExperience", PapyrusClassName, ClearBufferedExperience, registry));
	registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("ClearBufferedExperienceBySkill", PapyrusClassName, ClearBufferedExperienceBySkill, registry));
	
	registry->SetFunctionFlags(PapyrusClassName, "GetBufferedPointsBySkill", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags(PapyrusClassName, "MultiplyBufferedExperience", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags(PapyrusClassName, "MultiplyBufferedExperienceBySkill", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags(PapyrusClassName, "ClearBufferedExperience", VMClassRegistry::kFunctionFlag_NoWait);
	registry->SetFunctionFlags(PapyrusClassName, "ClearBufferedExperienceBySkill", VMClassRegistry::kFunctionFlag_NoWait);

	return true;
}

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Sleep To Gain Experience Plugin";
		info->version = 3;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor) {
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (skse->runtimeVersion != CURRENT_RELEASE_RUNTIME) {
			_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);

			return false;
		}

		// get the serialization interface and query its version
		g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
		if (!g_serialization)
		{
			_MESSAGE("couldn't get serialization interface");

			return false;
		}
		if (g_serialization->version < SKSESerializationInterface::kVersion)
		{
			_MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);

			return false;
		}

		// get the messaging interface and query its version
		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		if (!g_messaging) {
			_MESSAGE("\tcouldn't get messaging interface");

			return false;
		}
		if (g_messaging->kInterfaceVersion < SKSEMessagingInterface::kInterfaceVersion) {
			_MESSAGE("\tmessaging interface too old (%d expected %d)", g_messaging->interfaceVersion, SKSEMessagingInterface::kInterfaceVersion);

			return false;
		}

		// get the papyrus interface and query its version
		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
		if (!g_papyrus) {
			_MESSAGE("\tcouldn't get papyrus interface");

			return false;
		}
		if (g_papyrus->interfaceVersion < SKSEPapyrusInterface::kInterfaceVersion) {
			_MESSAGE("\tpapyrus interface too old (%d expected %d)", g_papyrus->interfaceVersion, SKSEPapyrusInterface::kInterfaceVersion);

			return false;
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		_MESSAGE("SKSEPlugin_Query end");

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface* skse) {
		_MESSAGE("SKSEPlugin_Load begin");

		char stringBuffer[16];

		// configuration
		constexpr const char* configFileName = "Data\\SKSE\\Plugins\\SleepToGainExperience.ini";
		constexpr const char* sectionName = "General";

		// general
		g_settings.enableSleepTimeRequirement = GetPrivateProfileInt(sectionName, "bEnableSleepTimeRequirement", 0, configFileName);
		g_settings.minDaysSleepNeeded = fabsf((float)GetPrivateProfileInt(sectionName, "iMinHoursSleepNeeded", 7, configFileName)) / 24.0f;
		GetPrivateProfileString(sectionName, "fPercentRequiresSleep", "1.0", stringBuffer, sizeof(stringBuffer), configFileName);
		g_settings.percentExpRequiresSleep = fminf(1.0f, fabsf(strtof(stringBuffer, nullptr)));
		GetPrivateProfileString(sectionName, "fInteruptedPenaltyPercent", "1.0", stringBuffer, sizeof(stringBuffer), configFileName);
		g_settings.interuptedPenaltyPercent = fminf(1.0f, fabsf(strtof(stringBuffer, nullptr)));

		// We can't write the hook here as g_thePlayer is not initialized so instead we do it when all forms have been loaded.
		g_messaging->RegisterListener(g_pluginHandle, "SKSE", Messaging_Callback);

		// register callbacks and unique ID for serialization

		// ### this must be a UNIQUE ID, change this and email me the ID so I can let you know if someone else has already taken it
		g_serialization->SetUniqueID(g_pluginHandle, 'StGE');

		g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
		g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
		g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

		g_papyrus->Register(Papyrus_RegisterFunctions);

		_MESSAGE("SKSEPlugin_Load end");

		return true;
	}

};
