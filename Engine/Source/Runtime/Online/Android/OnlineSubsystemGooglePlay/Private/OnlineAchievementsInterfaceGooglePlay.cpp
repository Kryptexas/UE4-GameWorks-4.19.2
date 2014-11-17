// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"

FOnlineAchievementsGooglePlay::FOnlineAchievementsGooglePlay( FOnlineSubsystemGooglePlay* InSubsystem )
	: AndroidSubsystem(InSubsystem)
{
}

void FOnlineAchievementsGooglePlay::QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound( PlayerId, false );
}


void FOnlineAchievementsGooglePlay::QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate)
{
	// Just query achievements to get descriptions
	// FIXME: This feels a little redundant, but we can see how platforms evolve, and make a decision then
	QueryAchievements( PlayerId, Delegate );
}

void FOnlineAchievementsGooglePlay::WriteAchievements( const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate )
{
	for (FStatPropertyArray::TConstIterator It(WriteObject->Properties); It; ++It)
	{
		// Access the stat and the value.
		const FVariantData& Stat = It.Value();

		// Create an achievement object which should be reported to the server.
		const FString AchievementName(It.Key().ToString());
		float PercentComplete = 0.0f;

		// Setup the percentage complete with the value we are writing from the variant type
		switch (Stat.GetType())
		{
		case EOnlineKeyValuePairDataType::Int32:
			{
				int32 Value;
				Stat.GetValue(Value);
				PercentComplete = (float)Value;
				break;
			}
		case EOnlineKeyValuePairDataType::Float:
			{
				float Value;
				Stat.GetValue(Value);
				PercentComplete = Value;
				break;
			}
		default:
			{
				UE_LOG(LogOnline, Error, TEXT("FOnlineAchievementsGooglePlay Trying to write an achievement with incompatible format. Not a float or int"));
				break;
			}
		}

		// Find the ID in the mapping
		auto DefaultSettings = GetDefault<UAndroidRuntimeSettings>();

		for(const auto& Mapping : DefaultSettings->AchievementMap)
		{
			if(Mapping.Name == AchievementName)
			{
				UE_LOG(LogOnline, Log, TEXT("Writing achievement name: %s, id: %s, progress: %.0f"),
					*Mapping.Name, *Mapping.AchievementID, PercentComplete);
				extern void AndroidThunkCpp_WriteAchievement(const FString& AchievementID, float PercentComplete);
				AndroidThunkCpp_WriteAchievement(Mapping.AchievementID, PercentComplete);
				break;
			}
		}
	}
}

EOnlineCachedResult::Type FOnlineAchievementsGooglePlay::GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements)
{
	// look up achievements for player
	OutAchievements = Achievements;

	// did we have them cached?
	return OutAchievements.Num() > 0 ? EOnlineCachedResult::Success : EOnlineCachedResult::NotFound;
}


EOnlineCachedResult::Type FOnlineAchievementsGooglePlay::GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	FOnlineAchievementDesc* FoundDesc = AchievementDescriptions.Find( AchievementId );
	if( FoundDesc != NULL )
	{
		OutAchievementDesc = *FoundDesc;
	}
	return FoundDesc != NULL ? EOnlineCachedResult::Success : EOnlineCachedResult::NotFound;
}

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsGooglePlay::ResetAchievements( const FUniqueNetId& PlayerId )
{
	check(!TEXT("ResetAchievements has not been implemented"));
	return false;
};
#endif // !UE_BUILD_SHIPPING

EOnlineCachedResult::Type FOnlineAchievementsGooglePlay::GetCachedAchievement( const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement )
{
	for(auto& CurrentAchievement : Achievements)
	{
		if(CurrentAchievement.Id == AchievementId)
		{
			OutAchievement = CurrentAchievement;
			return EOnlineCachedResult::Success;
		}
	}

	return EOnlineCachedResult::NotFound;
}
