// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __AssetEditorManager_h__
#define __AssetEditorManager_h__

#include "IToolkit.h"	// For EToolkitMode
#include "Ticker.h" 	// For automation to request assets to load
#include "Messaging.h"
#include "UnrealEdMessages.h"


// This class keeps track of a currently open asset editor; allowing it to be
// brought into focus, closed, etc..., without concern for how the editor was
// implemented.
class UNREALED_API IAssetEditorInstance
{
public:
	virtual FName GetEditorName() const = 0;
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) = 0;
	virtual bool CloseWindow() = 0;
};


/**
 * Implements a manager for Editor windows that are currently open and the assets they are editing.
 *
 * @todo toolkit: Merge this functionality into FToolkitManager
 */
class UNREALED_API FAssetEditorManager
	: public FGCObject
{
public:

	// Get the singleton instance of the asset editor manager
	static FAssetEditorManager& Get();

	// Called when the editor is exiting to shutdown the manager
	void OnExit();

	// Tries to open an editor for the specified asset.  Returns true if the asset is open in an editor.
	//   If the file is already open in an editor, it will not create another editor window but instead bring it to front
	bool OpenEditorForAsset( UObject* Asset, const EToolkitMode::Type ToolkitMode = EToolkitMode::Standalone, TSharedPtr< class IToolkitHost > OpenedFromLevelEditor = TSharedPtr< IToolkitHost >() );
	bool OpenEditorForAssets( const TArray< UObject* >& Asset, const EToolkitMode::Type ToolkitMode = EToolkitMode::Standalone, TSharedPtr< class IToolkitHost > OpenedFromLevelEditor = TSharedPtr< IToolkitHost >() );

	// Returns the editor if there is already an editor open for the specified asset.
	// If there is one open and bFocusIfOpen is true, that editor will be brought to the foreground and focused if possible.
	IAssetEditorInstance* FindEditorForAsset(UObject* Asset, bool bFocusIfOpen);

	// Close any editor which is not this one
	void CloseOtherEditors(UObject* Asset, IAssetEditorInstance* OnlyEditor);

	/** Get all assets currently being tracked with open editors */
	TArray<UObject*> GetAllEditedAssets();

	// Notify the asset editor manager that an asset was opened
	void NotifyAssetOpened(UObject* Asset, IAssetEditorInstance* Instance);
	void NotifyAssetsOpened( const TArray< UObject* >& Assets, IAssetEditorInstance* Instance);

	// Notify the asset editor manager that an asset editor is done editing an asset
	void NotifyAssetClosed(UObject* Asset, IAssetEditorInstance* Instance);

	// Notify the asset editor manager that an asset was closed
	void NotifyEditorClosed(IAssetEditorInstance* Instance);

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	// Close all open asset editors
	bool CloseAllAssetEditors();
	
	/** Called when an asset editor is requested to be opened */
	DECLARE_EVENT_OneParam( FAssetEditorManager, FAssetEditorRequestOpenEvent, UObject* );
	virtual FAssetEditorRequestOpenEvent& OnAssetEditorRequestedOpen() { return AssetEditorRequestOpenEvent; }

	/** Called when an asset editor is actually opened */
	DECLARE_EVENT_OneParam(FAssetEditorManager, FAssetEditorOpenEvent, UObject*);
	FAssetEditorOpenEvent& OnAssetEditorOpened() { return AssetEditorOpenedEvent; }

	/** Restore the assets that were previously open when the editor was last closed */
	void RestorePreviouslyOpenAssets();

private:

	// Hidden default constructor since the asset editor manager is a singleton.
	FAssetEditorManager();


private:

	// Handles FAssetEditorRequestOpenAsset messages.
	void HandleRequestOpenAssetMessage( const FAssetEditorRequestOpenAsset& Message, const IMessageContextRef& Context );

	// Opens an asset by path
	void OpenEditorForAsset(const FString& AssetPathName);

	// Handles ticks from the ticker.
	bool HandleTicker( float DeltaTime );

	// Saves a list of open asset editors so they can be restored on editor restart
	void SaveOpenAssetEditors(bool bOnShutdown);

private:

	/** struct used by OpenedEditorTimes map to store editor names and times */
	struct FOpenedEditorTime
	{
		FName EditorName;
		FDateTime OpenedTime;
	};

	/**
	 * Holds the opened assets.
	 */
	TMultiMap<UObject*, IAssetEditorInstance*> OpenedAssets;

	/**
	 * Holds the opened editors.
	 */
	TMultiMap<IAssetEditorInstance*, UObject*> OpenedEditors;

	/**
	 * Holds the times that editors were opened.
	 */
	TMap<IAssetEditorInstance*, FOpenedEditorTime> OpenedEditorTimes;

	/**
	 * Holds the cumulative time editors have been open by type.
	 */
	TMap<FName, FTimespan> EditorDurations;

private:

	// The singleton instance
	static FAssetEditorManager* Instance;

	// Holds the messaging endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a delegate to be invoked when the widget ticks.
	FTickerDelegate TickDelegate;

	/** Multicast delegate executed when an asset editor is requested to be opened */
	FAssetEditorRequestOpenEvent AssetEditorRequestOpenEvent;

	/** Multicast delegate executed when an asset editor is actually opened */
	FAssetEditorOpenEvent AssetEditorOpenedEvent;

	/** Flag whether we are currently shutting down */
	bool bSavingOnShutdown;
};


#endif
