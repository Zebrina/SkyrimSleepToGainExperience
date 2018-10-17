scriptname SleepToGainExperience extends Quest

float function GetBufferedPointsBySkill(string avName) global native
function FlushBufferedExperience(float daysSlept, bool interupted) global native
function FlushBufferedExperienceBySkill(string avName, float daysSlept, bool interupted) global native
function MultiplyBufferedExperience(float mult) global native
function MultiplyBufferedExperienceBySkill(string avName, float mult) global native
function ClearBufferedExperience() global native
function ClearBufferedExperienceBySkill(string avName) global native

GlobalVariable Survival_ModeToggle = none
bool property SurvivalEnabled hidden
	bool function get()
		if (!Survival_ModeToggle && Game.GetModByName("ccqdrsse001-survivalmode.esl") != 0xFF)
			Survival_ModeToggle = Game.GetFormFromFile(0x000828, "ccqdrsse001-survivalmode.esl") as GlobalVariable
		endif
		return Survival_ModeToggle && Survival_ModeToggle.GetValue() != 0.0
	endfunction
endproperty

event OnInit()
	self.RegisterForSleep()
endevent

float _sleepStartTime = 0.0
event OnSleepStart(float sleepStartTime, float desiredSleepEndTime)
	if (SurvivalEnabled)
		_sleepStartTime = 0.0
		FlushBufferedExperience(desiredSleepEndTime - sleepStartTime, false)
	else
		_sleepStartTime = sleepStartTime
	endif
endevent
event OnSleepStop(bool interupted)
	if (_sleepStartTime != 0.0)
		FlushBufferedExperience(Utility.GetCurrentGameTime() - _sleepStartTime, interupted)
	endif
endevent
