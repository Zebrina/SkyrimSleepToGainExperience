scriptname SleepToGainExperience_DA extends daymoyl_QuestTemplate

bool function QuestCondition(Location akLocation, Actor akAggressor, Actor akFollower)
	SleepToGainExperience.ClearBufferedExperience()
	return false ; Always return false to pass along to other scenarios.
endfunction
