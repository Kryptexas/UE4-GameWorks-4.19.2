// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Module includes
#include "OnlineSubsystemIOSPrivatePCH.h"

// Game Center includes
#include <GameKit/GKAchievement.h>
#include <GameKit/GKAchievementDescription.h>


FOnlineAchievementsIOS::FOnlineAchievementsIOS( class FOnlineSubsystemIOS* InSubsystem )
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemIOS::FOnlineAchievementsIOS()"));
	IOSSubsystem = InSubsystem;
}


bool FOnlineAchievementsIOS::ReadAchievements( const FUniqueNetId& PlayerId )
{
	dispatch_async(dispatch_get_main_queue(), ^
	{
		[GKAchievement loadAchievementsWithCompletionHandler: ^(NSArray *loadedAchievements, NSError *error)
			{
				bool bSuccess = error == NULL;
				UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemIOS::loadAchievementsWithCompletionHandler - %d"), bSuccess);

				if( bSuccess )
				{
					UE_LOG(LogOnline, Display, TEXT("Loaded %d achievements"), [loadedAchievements count]);
					Achievements.Empty();

					for( int32 AchievementIdx = 0; AchievementIdx < [loadedAchievements count]; AchievementIdx++ )
					{
						GKAchievement* LoadedAchievement = [loadedAchievements objectAtIndex:AchievementIdx];
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement: %s"), ANSI_TO_TCHAR([LoadedAchievement.identifier cStringUsingEncoding:NSASCIIStringEncoding]));
					
						Achievements.AddZeroed();
						FOnlineAchievement& OnlineAchievement = Achievements[AchievementIdx];
						OnlineAchievement.Id = ANSI_TO_TCHAR([LoadedAchievement.identifier cStringUsingEncoding:NSASCIIStringEncoding]);
						OnlineAchievement.Progress = LoadedAchievement.percentComplete;
					}
				}
				else
				{
					UE_LOG(LogOnline, Display, TEXT("Failed to load achievements with error [%d]"), [error code]);
				}

				// Report back to the game thread whether this succeeded.
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
				{
					TriggerOnAchievementsReadDelegates(PlayerId, bSuccess);
					return true;
				}];
			}
		];
	});

	return true;
}


bool FOnlineAchievementsIOS::ReadAchievementDescriptions( const FUniqueNetId& PlayerId )
{
	dispatch_async(dispatch_get_main_queue(), ^
	{
		[GKAchievementDescription loadAchievementDescriptionsWithCompletionHandler: ^(NSArray *descriptions, NSError *error)
			{
				bool bSuccess = error == NULL;
				UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemIOS::loadAchievementDescriptionsWithCompletionHandler - %d"), bSuccess);

				if( bSuccess )
				{
					AchievementDescriptions.Empty();

					for( GKAchievementDescription* desc in descriptions )
					{
						FString Id = ANSI_TO_TCHAR([desc.identifier cStringUsingEncoding:NSASCIIStringEncoding]);
					
						FOnlineAchievementDesc OnlineAchievementDesc;
						OnlineAchievementDesc.Title = FText::FromString( ANSI_TO_TCHAR([desc.title cStringUsingEncoding:NSASCIIStringEncoding]) );
						OnlineAchievementDesc.LockedDesc = FText::FromString( ANSI_TO_TCHAR([desc.unachievedDescription cStringUsingEncoding:NSASCIIStringEncoding]) );
						OnlineAchievementDesc.UnlockedDesc = FText::FromString( ANSI_TO_TCHAR([desc.achievedDescription cStringUsingEncoding:NSASCIIStringEncoding]) );
						OnlineAchievementDesc.bIsHidden = desc.hidden;
					
						UE_LOG(LogOnline, Display, TEXT("============================================"));
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement id: %s"), *Id );
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement title: %s"), *OnlineAchievementDesc.Title.ToString() );
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement locked desc: %s"), *OnlineAchievementDesc.LockedDesc.ToString() );
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement unlocked desc: %s"), *OnlineAchievementDesc.UnlockedDesc.ToString() );
						UE_LOG(LogOnline, Display, TEXT("Loaded achievement hidden: %d"), OnlineAchievementDesc.bIsHidden );
						UE_LOG(LogOnline, Display, TEXT("============================================"));

						AchievementDescriptions.Add( Id, OnlineAchievementDesc );
					}
				}
				else
				{
					UE_LOG(LogOnline, Display, TEXT("Failed to load achievement descriptionss with error [%d]"), [error code]);
				}

				// Report back to the game thread whether this succeeded.
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
				{
					TriggerOnAchievementDescriptionsReadDelegates(PlayerId, bSuccess);
					return true;
				}];
			}
		];
	});
	return true;
}


bool FOnlineAchievementsIOS::WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& InWriteObject)
{
	// Hold a reference to the write object for the completion handler to write to.
	FOnlineAchievementsWriteRef WriteObject = InWriteObject;
	WriteObject->WriteState = EOnlineAsyncTaskState::InProgress;

	NSMutableArray* UnreportedAchievements = [NSMutableArray arrayWithCapacity:WriteObject->Properties.Num()];
	[UnreportedAchievements retain];

	int32 StatIdx = 0;
	for (FStatPropertyArray::TConstIterator It(WriteObject->Properties); It; ++It)
	{
		// Access the stat and the value.
		const FVariantData& Stat = It.Value();

		// Create an achievement object which should be reported to the server.
		NSString* AchievementID = [NSString stringWithCString:TCHAR_TO_UTF8( *NormalizeAchievementName( It.Key().ToString() ) ) encoding:NSUTF8StringEncoding];
		
		GKAchievement* Achievement = [[[GKAchievement alloc] initWithIdentifier:AchievementID] autorelease];

		// Setup the percentage complete with the value we are writing from the variant type
		switch (Stat.GetType())
		{
			case EOnlineKeyValuePairDataType::Int32:
			{
				int32 Value;
				Stat.GetValue(Value);
				Achievement.percentComplete = (float)Value;

				break;
			}
			case EOnlineKeyValuePairDataType::Float:
			{
				float Value;
				Stat.GetValue(Value);
				Achievement.percentComplete = Value;

				break;
			}
			default:
			{
				UE_LOG(LogOnline, Error, TEXT("FOnlineSubsystemIOS Trying to write an achievement with incompatible format. Not a float or int"));
				break;
			}
		}

		[UnreportedAchievements addObject:Achievement];
	}

	// flush the achievements to the server
	if( [UnreportedAchievements count] > 0 )
	{
		dispatch_async(dispatch_get_main_queue(), ^
		{
			[GKAchievement reportAchievements : UnreportedAchievements withCompletionHandler : ^ (NSError* Error)
			{
				if (Error == nil)
				{
					for (GKAchievement* achievement in UnreportedAchievements)
					{
						FString AchievementId = ANSI_TO_TCHAR([achievement.identifier cStringUsingEncoding : NSASCIIStringEncoding]);
						UE_LOG(LogOnline, Display, TEXT("Successfully reported achievement: %s, isComplete:%d"), *AchievementId, [achievement isCompleted] ? 1 : 0);

						if ([achievement isCompleted])
						{
							// Report back to the game thread whether this succeeded.
							[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
							{
								TriggerOnAchievementUnlockedDelegates(PlayerId, AchievementId);
								return true;
							}];
						}
					}
				}
				else
				{
					UE_LOG(LogOnline, Display, TEXT("Failed to report achievements with error[%d]"), [Error code]);
				}

				// Release any handle of the achievements back to the IOS gc
				[UnreportedAchievements release];

				// Report whether this succeeded or not back to whoever is listening.
				WriteObject->WriteState = Error == nil ? EOnlineAsyncTaskState::Done : EOnlineAsyncTaskState::Failed;
				// Report back to the game thread whether this succeeded.
				
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
				{
					TriggerOnAchievementsWrittenDelegates(PlayerId, Error == nil);
					return true;
				}];
			}
			];
		});
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No achievements were written to be flushed"));
		TriggerOnAchievementsWrittenDelegates( PlayerId, false );
		WriteObject->WriteState = EOnlineAsyncTaskState::Failed;
	}

	return WriteObject->WriteState != EOnlineAsyncTaskState::Failed;
}


bool FOnlineAchievementsIOS::GetAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)
{
	bool bHasMatchingAchievement = false;
	
	for( int32 AchievementIdx = 0; AchievementIdx < Achievements.Num(); AchievementIdx++ )
	{
		if( Achievements[AchievementIdx].Id == AchievementId )
		{
			OutAchievement = Achievements[AchievementIdx];
			bHasMatchingAchievement = true;
			break;
		}
	}

	return bHasMatchingAchievement;
}


bool FOnlineAchievementsIOS::GetAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements)
{
	// look up achievements for player
	OutAchievements = Achievements;

	// did we have them cached?
	return OutAchievements.Num() > 0;
}


bool FOnlineAchievementsIOS::GetAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	FOnlineAchievementDesc* FoundDesc = AchievementDescriptions.Find( AchievementId );
	if( FoundDesc != NULL )
	{
		OutAchievementDesc = *FoundDesc;
	}
	return FoundDesc != NULL;
}


FString FOnlineAchievementsIOS::NormalizeAchievementName( const FString& InAchievementId )
{
	FString Id = InAchievementId;
	if( Id.Find( TEXT(".") ) == INDEX_NONE )
	{
		Id = FString::Printf( TEXT("grp.%s"), *InAchievementId );
	}
	return Id;
}

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsIOS::ResetAchievements( const FUniqueNetId& PlayerId )
{
	check(!TEXT("ResetAchievements has not been implemented for IOS"));
	return false;
};
#endif // !UE_BUILD_SHIPPING
