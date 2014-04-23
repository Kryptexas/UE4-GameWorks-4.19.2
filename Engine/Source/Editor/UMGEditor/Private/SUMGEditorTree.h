// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"

//TODO rename SUMGEditorHierarchy
class SUMGEditorTree : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTree ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<IUMGEditor> UMGEditor, UGUIPage* ObjectToEdit);
	~SUMGEditorTree();
	
	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	/** Set the parent tab of the viewport for determining visibility */
	void SetParentTab( TSharedRef<SDockTab> InParentTab ) { ParentTab = InParentTab; }

private:
	/** Callback for updating preview socket meshes if the gui page has been modified. */
	void OnObjectPropertyChanged(UObject* ObjectBeingModified);
private:
	
	/** The parent tab where this viewport resides */
	TWeakPtr<SDockTab> ParentTab;

	/** Pointer back to the UMG editor tool that owns us */
	TWeakPtr<IUMGEditor> UMGEditorPtr;

	/** gui page being edited */
	UGUIPage* Page;

	TSharedPtr< STreeView< TSharedPtr< FString > > > PageTreeView;
};
