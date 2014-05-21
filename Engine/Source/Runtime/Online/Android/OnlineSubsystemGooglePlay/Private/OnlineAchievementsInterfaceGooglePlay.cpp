// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "OnlineAsyncTaskManagerGooglePlay.h"
#include "Online.h"
#include "Android/AndroidJNI.h"

FOnlineAchievementsGooglePlay::FPendingAchievementQuery FOnlineAchievementsGooglePlay::PendingAchievementQuery;

extern "C" void Java_com_epicgames_ue4_GameActivity_nativeUpdateAchievements(JNIEnv* LocalJNIEnv, jobject LocalThiz, jobjectArray Achievements)
{
	TArray<FOnlineAchievement> GooglePlayAchievements;

	auto ArrayLength = LocalJNIEnv->GetArrayLength(Achievements);

	for(jsize i = 0; i < ArrayLength; ++i)
	{
		jobject Entry = LocalJNIEnv->GetObjectArrayElement(Achievements, i);

		jobject StringID = LocalJNIEnv->GetObjectField(Entry, JDef_GameActivity::AchievementIDField);
		jdouble Progress = LocalJNIEnv->GetDoubleField(Entry, JDef_GameActivity::AchievementProgressField);

		FOnlineAchievement NewAchievement;

		const char* StringIDChars = LocalJNIEnv->GetStringUTFChars((jstring)StringID, nullptr);
		NewAchievement.Id = FString(StringIDChars);
		LocalJNIEnv->ReleaseStringUTFChars((jstring)StringID, StringIDChars);

		NewAchievement.Progress = Progress;

		GooglePlayAchievements.Add(NewAchievement);
	}

	// Do the final copy of new achievements on the game thread
	auto AchievementsInterface = FOnlineAchievementsGooglePlay::PendingAchievementQuery.AchievementsInterface;
	
	if(!AchievementsInterface ||
	   !AchievementsInterface->AndroidSubsystem ||
	   !AchievementsInterface->AndroidSubsystem->GetAsyncTaskManager())
	{
		// We should call the delegate with a false parameter here, but if we don't have
		// the async task manager we're not going to call it on the game thread.
		return;
	}

	AchievementsInterface->AndroidSubsystem->GetAsyncTaskManager()->AddGenericToOutQueue([GooglePlayAchievements]()
	{
		auto& PendingQuery = FOnlineAchievementsGooglePlay::PendingAchievementQuery;

		PendingQuery.AchievementsInterface->Achievements = GooglePlayAchievements;

		// Must convert the Google Play IDs to the names specified by the game.
		auto DefaultSettings = GetDefault<UAndroidRuntimeSettings>();
		for(auto& Achievement : PendingQuery.AchievementsInterface->Achievements)
		{
			for(const auto& Mapping : DefaultSettings->AchievementMap)
			{
				if(Mapping.AchievementID == Achievement.Id)
				{
					Achievement.Id = Mapping.Name;
					break;
				}
			}
		}

		PendingQuery.Delegate.ExecuteIfBound( PendingQuery.PlayerID, true );
		PendingQuery.IsQueryPending = false;
	});
}

FOnlineAchievementsGooglePlay::FOnlineAchievementsGooglePlay( FOnlineSubsystemGooglePlay* InSubsystem )
	: AndroidSubsystem(InSubsystem)
{
	PendingAchievementQuery.AchievementsInterface = this;
}

void FOnlineAchievementsGooglePlay::QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate)
{
	if(PendingAchievementQuery.IsQueryPending)
	{
		UE_LOG(LogOnline, Warning, TEXT("FOnlineAchievementsGooglePlay::QueryAchievements: Cannot start a new query while one is in progress."));
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	PendingAchievementQuery.Delegate = Delegate;
	PendingAchievementQuery.PlayerID = FUniqueNetIdString(PlayerId);
	PendingAchievementQuery.IsQueryPending = true;

	extern void AndroidThunkCpp_QueryAchievements();
	AndroidThunkCpp_QueryAchievements();
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
