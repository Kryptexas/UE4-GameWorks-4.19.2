// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

#include "OnlineSessionInterface.h"


APartyBeaconClient::APartyBeaconClient(const class FPostConstructInitializeProperties& PCIP) :
	Super(PCIP),
	bPendingReservationSent(false),
	bCancelReservation(false)
{
}

void APartyBeaconClient::RequestReservation(const FOnlineSessionSearchResult& DesiredHost, const FUniqueNetIdRepl& RequestingPartyLeader, const TArray<FPlayerReservation>& PartyMembers)
{
	bool bSuccess = false;
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			FString ConnectInfo;
			if (SessionInt->GetResolvedConnectString(DesiredHost, BeaconPort, ConnectInfo))
			{
				FURL ConnectURL(NULL, *ConnectInfo, TRAVEL_Absolute);
				if (InitClient(ConnectURL))
				{
					PendingReservation.PartyLeader = RequestingPartyLeader;
					PendingReservation.PartyMembers = PartyMembers;
					bPendingReservationSent = false;
					bSuccess = true;
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("RequestReservation: Failure to init client beacon with %s."), *ConnectURL.ToString());
				}
			}
		}
	}
	if (!bSuccess)
	{
		OnFailure();
	}
}

void APartyBeaconClient::CancelReservation()
{
	if (PendingReservation.PartyLeader.IsValid())
	{
		bCancelReservation = true;
		if (bPendingReservationSent)
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Sending cancel reservation request."));
			ServerCancelReservationRequest(PendingReservation.PartyLeader);
		}
		else
		{
			UE_LOG(LogBeacon, Verbose, TEXT("Reservation request never sent, no need to send cancelation request."));
			ReservationRequestComplete.ExecuteIfBound( EPartyReservationResult::ReservationRequestCanceled );
		}
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Unable to cancel reservation request with invalid party leader."));
	}
}

void APartyBeaconClient::OnConnected()
{
	if (!bCancelReservation)
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Party beacon connection established, sending join reservation request."));
		ServerReservationRequest(PendingReservation);
		bPendingReservationSent = true;
	}
	else
	{
		UE_LOG(LogBeacon, Verbose, TEXT("Reservation request previously canceled, aborting reservation request."));
		ReservationRequestComplete.ExecuteIfBound( EPartyReservationResult::ReservationRequestCanceled );
	}
}

void APartyBeaconClient::OnFailure()
{
	UE_LOG(LogBeacon, Verbose, TEXT("Party beacon connection failure, handling connection timeout."));
	HostConnectionFailure.ExecuteIfBound();
	Super::OnFailure();
}

bool APartyBeaconClient::ServerReservationRequest_Validate(FPartyReservation Reservation)
{
	return true;
}

void APartyBeaconClient::ServerReservationRequest_Implementation(FPartyReservation Reservation)
{
	APartyBeaconHost* BeaconHost = Cast<APartyBeaconHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		PendingReservation = Reservation;
		BeaconHost->ProcessReservationRequest(this, Reservation);
	}
}

bool APartyBeaconClient::ServerCancelReservationRequest_Validate(FUniqueNetIdRepl PartyLeader)
{
	return true;
}

void APartyBeaconClient::ServerCancelReservationRequest_Implementation(FUniqueNetIdRepl PartyLeader)
{
	APartyBeaconHost* BeaconHost = Cast<APartyBeaconHost>(GetBeaconOwner());
	if (BeaconHost)
	{
		bCancelReservation = true;
		BeaconHost->ProcessCancelReservationRequest(this, PartyLeader);
	}
}

void APartyBeaconClient::ClientReservationResponse_Implementation(EPartyReservationResult::Type ReservationResponse)
{
	UE_LOG(LogBeacon, Verbose, TEXT("Party beacon response received %s"), EPartyReservationResult::ToString(ReservationResponse));
	ReservationRequestComplete.ExecuteIfBound(ReservationResponse);
}
