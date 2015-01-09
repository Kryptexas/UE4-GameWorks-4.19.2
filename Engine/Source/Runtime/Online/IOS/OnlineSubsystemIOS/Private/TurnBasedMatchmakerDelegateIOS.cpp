// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemIOSPrivatePCH.h"

#include "OnlineTurnBasedInterface.h"
#include "TurnBasedMatchmakerDelegateIOS.h"

@interface FTurnBasedMatchmakerDelegateIOS()
{
	FTurnBasedMatchmakerDelegate* _delegate;
}

@end

@implementation FTurnBasedMatchmakerDelegateIOS

- (id)initWithDelegate:(FTurnBasedMatchmakerDelegate*)delegate
{
	if (self = [super init])
	{
		_delegate = delegate;
	}
	return self;
}

-(void)turnBasedMatchmakerViewController:(GKTurnBasedMatchmakerViewController*)viewController didFailWithError : (NSError*)error
{
	if (_delegate) {
		_delegate->OnMatchmakerFailed();
	}
}

-(void)turnBasedMatchmakerViewController:(GKTurnBasedMatchmakerViewController*)viewController didFindMatch : (GKTurnBasedMatch*)match
{
	if (_delegate) {
		[match loadMatchDataWithCompletionHandler : ^ (NSData* matchData, NSError* matchLoadError) {
			if (matchLoadError) {
				[self turnBasedMatchmakerViewController : viewController didFailWithError : matchLoadError];
				return;
			}

			NSMutableArray* playerIdentifierArray = [NSMutableArray array];
			for (GKTurnBasedParticipant* participant in match.participants) {
				if (!participant.playerID)
				{
					break;
				}
				[playerIdentifierArray addObject : participant.playerID];
			}
			[GKPlayer loadPlayersForIdentifiers : playerIdentifierArray withCompletionHandler : ^ (NSArray *players, NSError *nameLoadError) {
				if (nameLoadError) {
					[self turnBasedMatchmakerViewController : viewController didFailWithError : nameLoadError];
					return;
				}

				FTurnBasedMatchRef MatchRef(new FTurnBasedMatchIOS(match, players));
				_delegate->OnMatchFound(MatchRef);
			}];
		}];
	}
	else {
		[match removeWithCompletionHandler : nil];
	}
}

-(void)turnBasedMatchmakerViewController:(GKTurnBasedMatchmakerViewController*)viewController playerQuitForMatch : (GKTurnBasedMatch*)match
{
}

-(void)turnBasedMatchmakerViewControllerWasCancelled : (GKTurnBasedMatchmakerViewController*)viewController
{
	if (_delegate) {
		_delegate->OnMatchmakerCancelled();
	}
}

@end
