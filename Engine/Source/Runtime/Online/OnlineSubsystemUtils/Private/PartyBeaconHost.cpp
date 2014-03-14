// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "Online.h"

APartyBeaconHost::APartyBeaconHost(const FPostConstructInitializeProperties& PCIP) :
	Super(PCIP),
	SessionName(NAME_None),
	NumConsumedReservations(0),
	MaxReservations(0)
{
	PrimaryActorTick.bCanEverTick = true;
}

void APartyBeaconHost::Tick(float DeltaTime)
{
	IOnlineSessionPtr SessionsInt = Online::GetSessionInterface();
	if (SessionsInt.IsValid())
	{
		FNamedOnlineSession* Session = SessionsInt->GetNamedSession(SessionName);
		if (Session)
		{
			TArray< TSharedPtr<FUniqueNetId> > PlayersToLogout;
			for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
			{
				FPartyReservation& PartyRes = Reservations[ResIdx];

				// Determine if we have a client connection for the reservation 
				bool bIsConnectedReservation = false;
				for (int32 ClientIdx=0; ClientIdx < ClientActors.Num(); ClientIdx++)
				{
					APartyBeaconClient* Client = Cast<APartyBeaconClient>(ClientActors[ClientIdx]);
					if (Client)
					{
						const FPartyReservation& ClientRes = Client->GetPendingReservation();
						if (ClientRes.PartyLeader == PartyRes.PartyLeader)
						{
							bIsConnectedReservation = true;
							break;
						}
					}
					else
					{
						UE_LOG(LogBeacon, Error, TEXT("Missing PartyBeaconClient found in ClientActors array"));
						ClientActors.RemoveAtSwap(ClientIdx);
						ClientIdx--;
					}
				}

				// Don't update clients that are still connected
				if (bIsConnectedReservation)
				{
					for (int32 PlayerIdx=0; PlayerIdx < PartyRes.PartyMembers.Num(); PlayerIdx++)
					{
						FPlayerReservation& PlayerEntry = PartyRes.PartyMembers[PlayerIdx];
						PlayerEntry.ElapsedTime = 0.0f;
					}
				}
				// Once a client disconnects, update the elapsed time since they were found as a registrant in the game session
				else
				{
					for (int32 PlayerIdx=0; PlayerIdx < PartyRes.PartyMembers.Num(); PlayerIdx++)
					{
						FPlayerReservation& PlayerEntry = PartyRes.PartyMembers[PlayerIdx];

						// Determine if the player is the owner of the session	
						const bool bIsSessionOwner = Session->OwningUserId.IsValid() && (*Session->OwningUserId == *PlayerEntry.UniqueId);

						// Determine if the player member is registered in the game session
						if (SessionsInt->IsPlayerInSession(SessionName, *PlayerEntry.UniqueId) ||
							// Never timeout the session owner
							bIsSessionOwner)
						{
							FUniqueNetIdMatcher PlayerMatch(*PlayerEntry.UniqueId);
							int32 FoundIdx = PlayersPendingJoin.FindMatch(PlayerMatch);
							if (FoundIdx != INDEX_NONE)
							{
								UE_LOG(LogBeacon, Display, TEXT("Beacon (%s): pending player %s found in session (%s)."),
									*GetName(),
									*PlayerEntry.UniqueId->ToDebugString(),
									*SessionName.ToString());

								// reset elapsed time since found
								PlayerEntry.ElapsedTime = 0.0f;
								// also remove from pending join list
								PlayersPendingJoin.RemoveAtSwap(FoundIdx);
							}
						}
						else
						{
							// update elapsed time
							PlayerEntry.ElapsedTime += DeltaTime;

							// if the player is pending it's initial join then check against TravelSessionTimeoutSecs instead
							FUniqueNetIdMatcher PlayerMatch(*PlayerEntry.UniqueId);
							int32 FoundIdx = PlayersPendingJoin.FindMatch(PlayerMatch);
							const bool bIsPlayerPendingJoin = FoundIdx != INDEX_NONE;
							// if the timeout has been exceeded then add to list of players 
							// that need to be logged out from the beacon
							if ((bIsPlayerPendingJoin && PlayerEntry.ElapsedTime > TravelSessionTimeoutSecs) ||
								(!bIsPlayerPendingJoin && PlayerEntry.ElapsedTime > SessionTimeoutSecs))
							{
								PlayersToLogout.AddUnique(PlayerEntry.UniqueId.GetUniqueNetId());
							}
						}
					}
				}
			}

			// Logout any players that timed out
			for (int32 LogoutIdx=0; LogoutIdx < PlayersToLogout.Num(); LogoutIdx++)
			{
				bool bFound = false;
				const TSharedPtr<FUniqueNetId>& UniqueId = PlayersToLogout[LogoutIdx];
				float ElapsedSessionTime = 0.f;
				for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
				{
					FPartyReservation& PartyRes = Reservations[ResIdx];
					for (int32 PlayerIdx=0; PlayerIdx < PartyRes.PartyMembers.Num(); PlayerIdx++)
					{
						FPlayerReservation& PlayerEntry = PartyRes.PartyMembers[PlayerIdx];
						if (*PlayerEntry.UniqueId == *UniqueId)
						{
							ElapsedSessionTime = PlayerEntry.ElapsedTime;
							bFound = true;
							break;
						}
					}

					if (bFound)
					{
						break;
					}
				}

				UE_LOG(LogBeacon, Display, TEXT("Beacon (%s): player logout due to timeout for %s, elapsed time = %0.3f"),
					*GetName(),
					*UniqueId->ToDebugString(),
					ElapsedSessionTime);
				// Also remove from pending join list
				PlayersPendingJoin.RemoveSingleSwap(UniqueId);
				// let the beacon handle the logout and notifications/delegates
				FUniqueNetIdRepl RemovedId(UniqueId);
				HandlePlayerLogout(RemovedId);
			}
		}
	}
}

bool APartyBeaconHost::InitHost()
{
	if(AOnlineBeaconHost::InitHost())
	{
		OnBeaconConnected(FName(TEXT("PartyBeacon"))).BindUObject(this, &APartyBeaconHost::ClientConnected);
		return true;
	}

	return false;
}

bool APartyBeaconHost::InitHostBeacon(int32 InTeamCount, int32 InTeamSize, int32 InMaxReservations, FName InSessionName)
{
	UE_LOG(LogBeacon, Verbose, TEXT("InitPartyHost TeamCount:%d TeamSize:%d MaxSize:%d"), InTeamCount, InTeamSize, InMaxReservations);
	if (InMaxReservations > 0)
	{
		SessionName = InSessionName;

		NumTeams = InTeamCount;
		NumPlayersPerTeam = InTeamSize;
		MaxReservations = InMaxReservations;
		Reservations.Empty(MaxReservations);
		return InitHost();
	}

	return false;
}

void APartyBeaconHost::NewPlayerAdded(const FPlayerReservation& NewPlayer)
{
	UE_LOG(LogBeacon, Verbose, TEXT("Beacon adding player %s"), *NewPlayer.UniqueId->ToDebugString());

	PlayersPendingJoin.Add(NewPlayer.UniqueId.GetUniqueNetId());
}

void APartyBeaconHost::HandlePlayerLogout(const FUniqueNetIdRepl& PlayerId)
{
	if (PlayerId.IsValid())
	{
		bool bWasRemoved = false;

		UE_LOG(LogBeacon, Verbose, TEXT("HandlePlayerLogout %s"), *PlayerId->ToDebugString());
		for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
		{
			FPartyReservation& Reservation = Reservations[ResIdx];

			// find the player in an existing reservation slot
			for (int32 PlayerIdx=0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
			{	
				FPlayerReservation& PlayerEntry = Reservation.PartyMembers[PlayerIdx];
				if (PlayerEntry.UniqueId == PlayerId)
				{
					// player removed
					Reservation.PartyMembers.RemoveAtSwap(PlayerIdx--);
					bWasRemoved = true;

					// free up a consumed entry
					NumConsumedReservations--;
				}
			}

			// remove the entire party reservation slot if no more party members
			if (Reservation.PartyMembers.Num() == 0)
			{
				Reservations.RemoveAtSwap(ResIdx--);
			}

			if (bWasRemoved)
			{
				ReservationChanged.ExecuteIfBound();
				break;
			}
		}
	}
}

bool APartyBeaconHost::PlayerHasReservation(const FUniqueNetId& PlayerId) const
{
	bool bFound = false;

	for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FPartyReservation& ReservationEntry = Reservations[ResIdx];
		for (int32 PlayerIdx=0; PlayerIdx < ReservationEntry.PartyMembers.Num(); PlayerIdx++)
		{
			if (*ReservationEntry.PartyMembers[PlayerIdx].UniqueId == PlayerId)
			{
				bFound = true;
				break;
			}
		}
	}

	return bFound; 
}

bool APartyBeaconHost::GetPlayerValidation(const FUniqueNetId& PlayerId, FString& OutValidation) const
{
	bool bFound = false;
	OutValidation = FString();

	for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FPartyReservation& ReservationEntry = Reservations[ResIdx];
		for (int32 PlayerIdx=0; PlayerIdx < ReservationEntry.PartyMembers.Num(); PlayerIdx++)
		{
			if (*ReservationEntry.PartyMembers[PlayerIdx].UniqueId == PlayerId)
			{
				OutValidation = ReservationEntry.PartyMembers[PlayerIdx].ValidationStr;
				bFound = true;
				break;
			}
		}
	}

	return bFound; 
}

int32 APartyBeaconHost::GetExistingReservation(const FUniqueNetIdRepl& PartyLeader)
{
	int32 Result = INDEX_NONE;
	for (int32 ResIdx=0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FPartyReservation& ReservationEntry = Reservations[ResIdx];
		if (ReservationEntry.PartyLeader == PartyLeader)
		{
			Result = ResIdx;
			break;
		}
	}

	return Result; 
}

EPartyReservationResult::Type APartyBeaconHost::AddPartyReservation(const FPartyReservation& ReservationRequest)
{
	EPartyReservationResult::Type Result = EPartyReservationResult::GeneralError;

	int32 IncomingPartySize = ReservationRequest.PartyMembers.Num();
	if (NumConsumedReservations + IncomingPartySize <= MaxReservations)
	{
		const int32 ExistingReservationIdx = GetExistingReservation(ReservationRequest.PartyLeader);
		if (ExistingReservationIdx == INDEX_NONE)
		{
			bool bContinue = true;
			if (ValidatePlayers.IsBound())
			{
				bContinue = ValidatePlayers.Execute(ReservationRequest.PartyMembers);
			}

			if (bContinue)
			{
				NumConsumedReservations += IncomingPartySize;

				Reservations.Add(ReservationRequest);

				// Keep track of newly added players
				for (int32 Count = 0; Count < ReservationRequest.PartyMembers.Num(); Count++)
				{
					NewPlayerAdded(ReservationRequest.PartyMembers[Count]);
				}

				ReservationChanged.ExecuteIfBound();
				if (NumConsumedReservations == MaxReservations)
				{
					ReservationsFull.ExecuteIfBound();
				}

				Result = EPartyReservationResult::ReservationAccepted;
			}
			else
			{
				Result = EPartyReservationResult::ReservationDenied_Banned;
			}
		}
		else
		{
			Result = EPartyReservationResult::ReservationDuplicate;
		}
	}
	else
	{
		Result = EPartyReservationResult::PartyLimitReached;
	}

	return Result;
}

void APartyBeaconHost::RemovePartyReservation(const FUniqueNetIdRepl& PartyLeader)
{
	const int32 ExistingReservationIdx = GetExistingReservation(PartyLeader);
	if (ExistingReservationIdx != INDEX_NONE)
	{
		NumConsumedReservations -= Reservations[ExistingReservationIdx].PartyMembers.Num();
		Reservations.RemoveAtSwap(ExistingReservationIdx);

		CancelationReceived.ExecuteIfBound(*PartyLeader);
		ReservationChanged.ExecuteIfBound();
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("Failed to find reservation to cancel for leader %s:"), *PartyLeader->ToString());
	}
}

void APartyBeaconHost::ProcessReservationRequest(APartyBeaconClient* Client, const FPartyReservation& ReservationRequest)
{
	UE_LOG(LogBeacon, Verbose, TEXT("ProcessReservationRequest %s PartyLeader: %s from (%s)"), 
		Client ? *Client->GetName() : TEXT("NULL"), 
		ReservationRequest.PartyLeader.IsValid() ? *ReservationRequest.PartyLeader->ToString() : TEXT("INVALID"),
		Client ? *Client->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	if (Client)
	{
		EPartyReservationResult::Type Result = AddPartyReservation(ReservationRequest);
		Client->ClientReservationResponse(Result);
	}
}

void APartyBeaconHost::ProcessCancelReservationRequest(APartyBeaconClient* Client, const FUniqueNetIdRepl& PartyLeader)
{
	UE_LOG(LogBeacon, Verbose, TEXT("ProcessCancelReservationRequest %s PartyLeader: %s from (%s)"), 
		Client ? *Client->GetName() : TEXT("NULL"), 
		PartyLeader.IsValid() ? *PartyLeader->ToString() : TEXT("INVALID"),
		Client ? *Client->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	if (Client)
	{
		RemovePartyReservation(PartyLeader);
	}
}

void APartyBeaconHost::ClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	UE_LOG(LogBeacon, Verbose, TEXT("ClientConnected %s from (%s)"), 
		NewClientActor ? *NewClientActor->GetName() : TEXT("NULL"), 
		NewClientActor ? *NewClientActor->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));
}

AOnlineBeaconClient* APartyBeaconHost::SpawnBeaconActor()
{	
	FActorSpawnParameters SpawnInfo;
	APartyBeaconClient* BeaconActor = GetWorld()->SpawnActor<APartyBeaconClient>(APartyBeaconClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (BeaconActor)
	{
		BeaconActor->SetBeaconOwner(this);
	}

	return BeaconActor;
}

void APartyBeaconHost::DumpReservations() const
{
	FUniqueNetIdRepl NetId;
	FPlayerReservation PlayerRes;

	UE_LOG(LogBeacon, Display, TEXT("Debug info for Beacon: %s"), *BeaconName);
	UE_LOG(LogBeacon, Display, TEXT("Session that reservations are for: %s"), *SessionName.ToString());
	UE_LOG(LogBeacon, Display, TEXT("Number of teams: %d"), NumTeams);
	UE_LOG(LogBeacon, Display, TEXT("Number players per team: %d"), NumPlayersPerTeam);
	UE_LOG(LogBeacon, Display, TEXT("Number total reservations: %d"), MaxReservations);
	UE_LOG(LogBeacon, Display, TEXT("Number consumed reservations: %d"), NumConsumedReservations);
	UE_LOG(LogBeacon, Display, TEXT("Number of party reservations: %d"), Reservations.Num());

	// Log each party that has a reservation
	for (int32 PartyIndex = 0; PartyIndex < Reservations.Num(); PartyIndex++)
	{
		NetId = Reservations[PartyIndex].PartyLeader;
		UE_LOG(LogBeacon, Display, TEXT("  Party leader: %s"), *NetId->ToString());
		UE_LOG(LogBeacon, Display, TEXT("  Party team: %d"), Reservations[PartyIndex].TeamNum);
		UE_LOG(LogBeacon, Display, TEXT("  Party size: %d"), Reservations[PartyIndex].PartyMembers.Num());
		// Log each member of the party
		for (int32 MemberIndex = 0; MemberIndex < Reservations[PartyIndex].PartyMembers.Num(); MemberIndex++)
		{
			PlayerRes = Reservations[PartyIndex].PartyMembers[MemberIndex];
			UE_LOG(LogBeacon, Display, TEXT("  Party member: %s"), *PlayerRes.UniqueId->ToString());
		}
	}
	UE_LOG(LogBeacon, Display, TEXT(""));
}