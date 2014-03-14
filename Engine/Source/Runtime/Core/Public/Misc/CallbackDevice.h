// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CallbackDevice.h:  Allows the engine to do callbacks into the editor
=============================================================================*/

#pragma once

#include "ScopedCallback.h"

/**
 * The list of events that can be fired from the engine to unknown listeners
 */
enum ECallbackQueryType
{
	CALLBACK_ModalErrorMessage,				//Sent when FMessageDialog::Open or appDebugMesgf need to display a modal
	CALLBACK_LocalizationExportFilter,		// A query sent to find out if the provided object passes the localization export filter
	CALLBACK_QueryCount,
};

class AActor;

// delegates for hotfixes
namespace EHotfixDelegates
{
	enum Type
	{
		Test,
	};
}

// this is an example of a hotfix arg and return value structure. Once we have other examples, it can be deleted.
struct FTestHotFixPayload
{
	FString Message;
	bool ValueToReturn;
	bool Result;
};


class CORE_API FCoreDelegates
{
public:
	//hot fix delegate
	DECLARE_DELEGATE_TwoParams(FHotFixDelegate, void *, int32);

	// Callback for object property modifications
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnObjectPropertyChanged, UObject*);

	// Callback for object property modifications
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorLabelChanged, AActor*);

	// Callback for PreEditChange
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreObjectPropertyChanged, UObject*);

	#if WITH_EDITOR
	// Callback for when an asset is loaded (Editor)
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetLoaded, UObject*);
	#endif	//WITH_EDITOR

	// Callback for PER_MODULE_BOILERPLATE macro's GSerialNumberBlocksForDebugVisualizers
	DECLARE_DELEGATE_RetVal(int32***, FGetSerialNumberBlocksForDebugVisualizersDelegate);

	// Callback for PER_MODULE_BOILERPLATE macro's GObjectArrayForDebugVisualizers
	DECLARE_DELEGATE_RetVal(TArray<class UObjectBase*>*, FObjectArrayForDebugVisualizersDelegate);

	// Delegate type for redirector followed events ( Params: const FString& PackageName, UObject* Redirector )
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRedirectorFollowed, const FString&, UObject*);

	/** delegate type for opening a modal message box ( Params: const FString& Text, EAppMsgType::Type MessageType ) */
	DECLARE_DELEGATE_RetVal_TwoParams(EAppReturnType::Type, FOnModalMessageBox, const FText&, EAppMsgType::Type);

	/** delegate type for querrying whether a loaded object should replace an already exisiting one */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnLoadObjectsOnTop, const FString&);

	// Callback for handling an ensure
	DECLARE_MULTICAST_DELEGATE(FOnHandleSystemEnsure);

	// Callback for handling an error
	DECLARE_MULTICAST_DELEGATE(FOnHandleSystemError);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUserLoginChangedEvent, bool, int32);	

	// get a hotfix delegate
	static FHotFixDelegate& GetHotfixDelegate(EHotfixDelegates::Type HotFix);

	// Callback when a user logs in/out of the platform.
	static FOnUserLoginChangedEvent OnUserLoginChangedEvent;

	// Callback when an ensure has occurred
	static FOnHandleSystemEnsure OnHandleSystemEnsure;
	// Callback when an error (crash) has occurred
	static FOnHandleSystemError OnHandleSystemError;

	// Called before a property is changed
	static FOnPreObjectPropertyChanged OnPreObjectPropertyChanged;

	// Called when a property is changed
	static FOnObjectPropertyChanged OnObjectPropertyChanged;

	// Called when an actor label is changed
	static FOnActorLabelChanged OnActorLabelChanged;

#if WITH_EDITOR
	// Called when an asset is loaded
	static FOnAssetLoaded OnAssetLoaded;

	// Called before the editor displays a modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
	static FSimpleMulticastDelegate PreModal;

	// Called after the editor dismisses a modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
	static FSimpleMulticastDelegate PostModal;
#endif	//WITH_EDITOR

	// Called when SetCurrentCulture is called.
	static FSimpleMulticastDelegate OnCultureChanged;

	// Called when an error occured.
	static FSimpleMulticastDelegate OnShutdownAfterError;

	// Called when appInit is called.
	static FSimpleMulticastDelegate OnInit;

	// Called when the application is about to exit.
	static FSimpleMulticastDelegate OnExit;

	// Called when before the application is exiting.
	static FSimpleMulticastDelegate OnPreExit;

	// Called before garbage collection
	static FSimpleMulticastDelegate PreGarbageCollect;

	// Called after garbage collection
	static FSimpleMulticastDelegate PostGarbageCollect;

	// Called in PER_MODULE_BOILERPLATE macro.
	static FGetSerialNumberBlocksForDebugVisualizersDelegate GetSerialNumberBlocksDebugVisualizers;

	// Called in PER_MODULE_BOILERPLATE macro.
	static FObjectArrayForDebugVisualizersDelegate ObjectArrayForDebugVisualizers;

	// Sent when a UObjectRedirector was followed to find the destination object
	static FOnRedirectorFollowed	RedirectorFollowed;

	// Sent at the very beginning of LoadMap
	static FSimpleMulticastDelegate PreLoadMap;

	// Sent at the _successful_ end of LoadMap
	static FSimpleMulticastDelegate PostLoadMap;

	/** Color picker color has changed, please refresh as needed*/
	static FSimpleMulticastDelegate ColorPickerChanged;

	/** requests to open a message box */
	static FOnModalMessageBox ModalErrorMessage;

	/** querries whether an object should be loaded on top ( replace ) an already existing one */
	static FOnLoadObjectsOnTop ShouldLoadOnTop;

	/** called when loading a string asset reference */
	DECLARE_DELEGATE_OneParam(FStringAssetReferenceLoaded, FString const& /*LoadedAssetLongPathname*/);
	static FStringAssetReferenceLoaded StringAssetReferenceLoaded;

	/** called when path to world root is changed */
	DECLARE_MULTICAST_DELEGATE_OneParam(FPackageCreatedForLoad, class UPackage*);
	static FPackageCreatedForLoad PackageCreatedForLoad;

	/** called when saving a string asset reference, can replace the value with something else */
	DECLARE_DELEGATE_RetVal_OneParam( FString, FStringAssetReferenceSaving, FString const& /*SavingAssetLongPathname*/);
	static FStringAssetReferenceSaving StringAssetReferenceSaving;
		
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FWorldOriginOffset, class UWorld*, const FIntPoint&, const FIntPoint&);
	/** called before world origin shifting */
	static FWorldOriginOffset PreWorldOriginOffset;
	/** called after world origin shifting */
	static FWorldOriginOffset PostWorldOriginOffset;

	/** called when the main loop would otherwise starve. */
	DECLARE_DELEGATE(FStarvedGameLoop);
	static FStarvedGameLoop StarvedGameLoop;

	/** IOS-style application lifecycle delegates */
	DECLARE_MULTICAST_DELEGATE(FApplicationLifetimeDelegate);

	// This is called when the application is about to be deactivated (e.g., due to a phone call or SMS or the sleep button).
	// The game should be paused if possible, etc...
	static FApplicationLifetimeDelegate ApplicationWillDeactivateDelegate;
	
	// Called when the application has been reactivated (reverse any processing done in the Deactivate delegate)
	static FApplicationLifetimeDelegate ApplicationHasReactivatedDelegate;

	// This is called when the application is being backgrounded (e.g., due to switching
	// to another app or closing it via the home button)
	// The game should release shared resources, save state, etc..., since it can be
	// terminated from the background state without any further warning.
	static FApplicationLifetimeDelegate ApplicationWillEnterBackgroundDelegate; // for instance, hitting the home button

	// Called when the application is returning to the foreground (reverse any processing done in the EnterBackground delegate)
	static FApplicationLifetimeDelegate ApplicationHasEnteredForegroundDelegate;

	// This *may* be called when the application is getting terminated by the OS.
	// There is no guarantee that this will ever be called on a mobile device,
	// save state when ApplicationWillEnterBackgroundDelegate is called instead.
	static FApplicationLifetimeDelegate ApplicationWillTerminateDelegate;

private:
	// Callbacks for hotfixes
	static TArray<FHotFixDelegate> HotFixDelegates;
	// This class is only for namespace use
	FCoreDelegates() {}
};
