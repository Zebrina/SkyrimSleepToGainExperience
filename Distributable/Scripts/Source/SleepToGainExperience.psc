scriptname SleepToGainExperience extends Quest

function FlushBufferedExperience(float daysSlept, bool interupted) global native
function ClearBufferedExperience() global native

event OnInit()
	self.RegisterForSleep()
endevent

float _sleepStartTime = 0.0
event OnSleepStart(float sleepStartTime, float desiredSleepEndTime)
	_sleepStartTime = sleepStartTime
endevent
event OnSleepStop(bool interupted)
	FlushBufferedExperience(Utility.GetCurrentGameTime() - _sleepStartTime, interupted)
endevent
