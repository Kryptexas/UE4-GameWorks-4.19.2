// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Core includes.
#include "CorePrivate.h"

const FVector DefaultRefVector(0);


//////////////////////////////////////////////////////////////////////////
// FCoreDelegates

TArray<FCoreDelegates::FHotFixDelegate> FCoreDelegates::HotFixDelegates;

FCoreDelegates::FHotFixDelegate& FCoreDelegates::GetHotfixDelegate(EHotfixDelegates::Type HotFix)
{
	if (HotFix >= HotFixDelegates.Num())
	{
		HotFixDelegates.SetNum(HotFix + 1);
	}
	return HotFixDelegates[HotFix];
}

FCoreDelegates::FOnUserLoginChangedEvent FCoreDelegates::OnUserLoginChangedEvent;  
FCoreDelegates::FOnHandleSystemEnsure FCoreDelegates::OnHandleSystemEnsure;
FCoreDelegates::FOnHandleSystemError FCoreDelegates::OnHandleSystemError;
FCoreDelegates::FOnPreObjectPropertyChanged FCoreDelegates::OnPreObjectPropertyChanged;
FCoreDelegates::FOnObjectPropertyChanged FCoreDelegates::OnObjectPropertyChanged;
FCoreDelegates::FOnActorLabelChanged FCoreDelegates::OnActorLabelChanged;
#if WITH_EDITOR
	FCoreDelegates::FOnAssetLoaded FCoreDelegates::OnAssetLoaded;
	FSimpleMulticastDelegate FCoreDelegates::PreModal;
	FSimpleMulticastDelegate FCoreDelegates::PostModal;
#endif	//WITH_EDITOR
FSimpleMulticastDelegate FCoreDelegates::OnCultureChanged;
FSimpleMulticastDelegate FCoreDelegates::OnShutdownAfterError;
FSimpleMulticastDelegate FCoreDelegates::OnInit;
FSimpleMulticastDelegate FCoreDelegates::OnExit;
FSimpleMulticastDelegate FCoreDelegates::OnPreExit;
FSimpleMulticastDelegate FCoreDelegates::PreGarbageCollect;
FSimpleMulticastDelegate FCoreDelegates::PostGarbageCollect;
FCoreDelegates::FGetSerialNumberBlocksForDebugVisualizersDelegate FCoreDelegates::GetSerialNumberBlocksDebugVisualizers;
FCoreDelegates::FObjectArrayForDebugVisualizersDelegate FCoreDelegates::ObjectArrayForDebugVisualizers;
FCoreDelegates::FOnRedirectorFollowed FCoreDelegates::RedirectorFollowed;
FSimpleMulticastDelegate FCoreDelegates::PreLoadMap;
FSimpleMulticastDelegate FCoreDelegates::PostLoadMap;
FSimpleMulticastDelegate FCoreDelegates::ColorPickerChanged;
FCoreDelegates::FOnModalMessageBox FCoreDelegates::ModalErrorMessage;
FCoreDelegates::FOnLoadObjectsOnTop FCoreDelegates::ShouldLoadOnTop;
FCoreDelegates::FStringAssetReferenceLoaded FCoreDelegates::StringAssetReferenceLoaded;
FCoreDelegates::FStringAssetReferenceSaving FCoreDelegates::StringAssetReferenceSaving;
FCoreDelegates::FPackageCreatedForLoad FCoreDelegates::PackageCreatedForLoad;
FCoreDelegates::FWorldOriginOffset FCoreDelegates::PreWorldOriginOffset;
FCoreDelegates::FWorldOriginOffset FCoreDelegates::PostWorldOriginOffset;
FCoreDelegates::FStarvedGameLoop FCoreDelegates::StarvedGameLoop;

FCoreDelegates::FApplicationLifetimeDelegate FCoreDelegates::ApplicationWillDeactivateDelegate;
FCoreDelegates::FApplicationLifetimeDelegate FCoreDelegates::ApplicationHasReactivatedDelegate;
FCoreDelegates::FApplicationLifetimeDelegate FCoreDelegates::ApplicationWillEnterBackgroundDelegate;
FCoreDelegates::FApplicationLifetimeDelegate FCoreDelegates::ApplicationHasEnteredForegroundDelegate;
FCoreDelegates::FApplicationLifetimeDelegate FCoreDelegates::ApplicationWillTerminateDelegate; 
