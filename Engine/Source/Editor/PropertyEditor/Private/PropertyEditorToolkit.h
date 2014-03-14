// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class FPropertyEditorToolkit : public FAssetEditorToolkit
{
public:

	FPropertyEditorToolkit();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	virtual FName GetToolkitFName() const OVERRIDE;

	virtual FText GetBaseToolkitName() const OVERRIDE;

	virtual FText GetToolkitName() const OVERRIDE;

	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;

	virtual FLinearColor GetWorldCentricTabColorScale() const;

	bool IsExposedAsColumn( const TWeakPtr< IPropertyTreeRow >& Row ) const;

	void ToggleColumnForProperty( const TSharedPtr< class FPropertyPath >& PropertyPath );

	bool TableHasCustomColumns() const;

	virtual bool CloseWindow() OVERRIDE;

public:

	static TSharedRef<FPropertyEditorToolkit> CreateEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit );

	static TSharedRef<FPropertyEditorToolkit> CreateEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit );


private:

	void Initialize( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit );

	void CreatePropertyTree();

	void CreatePropertyTable();

	void CreateGridView();

	TSharedRef<SDockTab> SpawnTab_PropertyTree( const FSpawnTabArgs& Args );

	TSharedRef<SDockTab> SpawnTab_PropertyTable( const FSpawnTabArgs& Args ) ;

	void GridSelectionChanged();

	void GridRootPathChanged();

	void ConstructTreeColumns( const TSharedRef< class SHeaderRow >& HeaderRow );

	TSharedRef< SWidget > ConstructTreeCell( const FName& ColumnName, const TSharedRef< class IPropertyTreeRow >& Row );

	FReply OnToggleColumnClicked( const TWeakPtr< class IPropertyTreeRow > Row );

	const FSlateBrush* GetToggleColumnButtonImageBrush( const TWeakPtr< class IPropertyTreeRow > Row ) const;

	EVisibility GetToggleColumnButtonVisibility( const TSharedRef< class IPropertyTreeRow > Row ) const;

	void TickPinColorAndOpacity();

	FSlateColor GetPinColorAndOpacity( const TWeakPtr< IPropertyTreeRow > Row ) const;

	void TableColumnsChanged();

	EVisibility GetAddColumnInstructionsOverlayVisibility() const;


private:

	TSharedPtr< class SPropertyTreeViewImpl > PropertyTree;
	TSharedPtr< class IPropertyTable > PropertyTable;

	TSharedPtr< FPropertyPath > PathToRoot;

	TArray< TSharedRef< FPropertyPath > > PropertyPathsAddedAsColumns;

	/** Animation sequence to pulse the pin image */
	FCurveSequence PinSequence;
	FTimerDelegate TickPinColorDelegate;
	FSlateColor PinColor;
	TArray< TWeakPtr<IPropertyTreeRow> > PinRows;

	static const FName ApplicationId;
	static const FName TreeTabId;
	static const FName GridTabId;

	static const FName TreePinAsColumnHeaderId;
};