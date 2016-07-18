#include "SKSEMemUtil.h"

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameAPI.h"

#include "skse/GameReferences.h"
#include "skse/GameEvents.h"
#include "skse/GameTypes.h"
#include "skse/GameMenus.h"

#include "skse/PapyrusNativeFunctions.h"

#include <cmath>

constexpr char* PapyrusClassName = "SleepToGainExperience";

constexpr UInt32 pcVTableAdvanceSkillOffset = 0xF6;
//constexpr UInt32 pcVTableAdvanceSkillMemOffset = pcVTableAdvanceSkillOffset * 4;

typedef void (Actor::*Actor_AdvanceSkill_Method)(UInt32, float, UInt32, UInt32);
Actor_AdvanceSkill_Method Original_PlayerCharacter_AdvanceSkill = nullptr;

IDebugLog gLog("sleep_to_gain_experience.log");

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
	};

	float expBuf[(LastSkillId - FirstSkillId) + 1];

	inline SkillExperienceBuffer() {
		clear();
	}

	void addExperience(UInt32 skillId, float points) {
		if (skillId >= FirstSkillId && skillId <= LastSkillId) {
			expBuf[skillId - FirstSkillId] += points;
		}
	}

	void flushExperience(float percent) {
		for (UInt32 skillId = FirstSkillId; skillId <= LastSkillId; ++skillId) {
			float toAdd = expBuf[skillId - FirstSkillId] * percent;
			if (toAdd > 0.0f) {
				((**g_thePlayer).*Original_PlayerCharacter_AdvanceSkill)(skillId, toAdd, 0, 0);
				expBuf[skillId - FirstSkillId] -= toAdd;
			}
		}
	}

	void clear() {
		ZeroMemory(expBuf, sizeof(expBuf));
	}
};

SkillExperienceBuffer g_experienceBuffer;

void __fastcall PlayerCharacter_AdvanceSkill_Hooked(Actor* thisPtr, void*, UInt32 skillId, float points, UInt32 unk1, UInt32 unk2) {
	static BSFixedString ConsoleMenu = "Console";
	static BSFixedString ConsoleNativeUIMenu = "Console Native UI Menu";

	MenuManager* menuManager = MenuManager::GetSingleton();
	if (skillId >= SkillExperienceBuffer::FirstSkillId && skillId <= SkillExperienceBuffer::LastSkillId &&
		thisPtr == *g_thePlayer &&
		menuManager && !menuManager->IsMenuOpen(&ConsoleMenu) && !menuManager->IsMenuOpen(&ConsoleNativeUIMenu)) {

		//_MESSAGE("handle call");

		g_experienceBuffer.addExperience(skillId, g_settings.percentExpRequiresSleep * points);

		// call the original function but with points = 0
		// not sure what effect not calling it might have
		((*thisPtr).*Original_PlayerCharacter_AdvanceSkill)(skillId, (1.0f - g_settings.percentExpRequiresSleep) * points, unk1, unk2);
	}
	else {
		//_MESSAGE("ignore call");

		((*thisPtr).*Original_PlayerCharacter_AdvanceSkill)(skillId, points, unk1, unk2);
	}

	//_MESSAGE("AdvanceSkill_Hooked end");
}

void Serialization_Revert(SKSESerializationInterface* serializationInterface)
{
	_MESSAGE("Serialization_Revert begin");

	g_experienceBuffer.clear();

	_MESSAGE("Serialization_Revert end");
}

constexpr UInt32 serializationDataVersion = 1;

void Serialization_Save(SKSESerializationInterface* serializationInterface) {
	_MESSAGE("Serialization_Save begin");

	if (serializationInterface->OpenRecord('DATA', serializationDataVersion)) {
		serializationInterface->WriteRecordData(&g_experienceBuffer, sizeof(g_experienceBuffer));

		_MESSAGE("saved");
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
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded) {
		// All forms are loaded

		_MESSAGE("kMessage_DataLoaded begin");

		Original_PlayerCharacter_AdvanceSkill = SKSEMemUtil::WriteVTableHook(_GetObjectVTable(*g_thePlayer), pcVTableAdvanceSkillOffset, PlayerCharacter_AdvanceSkill_Hooked);

		/*
		// Retrieve points to vtable for PlayerCharacter.
		UInt32* pcVTable = (UInt32*)(*((UInt32*)(*g_thePlayer)));

		// Save old AdvanceSkill method.
		*reinterpret_cast<UInt32*>(&PlayerCharacter_AdvanceSkill) = (UInt32)pcVTable[pcVTableAdvanceSkillOffset];

		// Let me write my hook!
		UInt32 oldProtect = 0;
		VirtualProtect(pcVTable + pcVTableAdvanceSkillMemOffset, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);

		// Replace old AdvanceSkill method with my AdvanceSkill_Hooked function. 
		pcVTable[pcVTableAdvanceSkillOffset] = (UInt32)AdvanceSkill_Hooked;

		// I'm done writing.
		VirtualProtect(pcVTable + pcVTableAdvanceSkillMemOffset, sizeof(void*), oldProtect, nullptr);
		*/

		_MESSAGE("kMessage_DataLoaded end");
	}
}

void FlushBufferedExperience(StaticFunctionTag*, float daysSlept, bool interupted) {
	//_MESSAGE("FlushBufferedExperience begin");

	g_experienceBuffer.flushExperience((g_settings.enableSleepTimeRequirement ? fminf(1.0f, daysSlept / g_settings.minDaysSleepNeeded) : 1.0f) * (interupted ? g_settings.interuptedPenaltyPercent : 1.0f));

	//_MESSAGE("FlushBufferedExperience end");
}
void ClearBufferedExperience(StaticFunctionTag*) {
	g_experienceBuffer.clear();
}

bool Papyrus_RegisterFunctions(VMClassRegistry* registry) {
	registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, float, bool>("FlushBufferedExperience", PapyrusClassName, FlushBufferedExperience, registry));
	registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ClearBufferedExperience", PapyrusClassName, ClearBufferedExperience, registry));
	registry->SetFunctionFlags(PapyrusClassName, "ClearBufferedExperience", VMClassRegistry::kFunctionFlag_NoWait);

	return true;
}

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
		_MESSAGE("SKSEPlugin_Query begin");

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Sleep To Gain Experience Plugin";
		info->version = 2;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor) {
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
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
