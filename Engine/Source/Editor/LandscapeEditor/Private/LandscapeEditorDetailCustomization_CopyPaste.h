// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeEditorDetailCustomization_Base.h"


/**
 * Slate widgets customizer for the "Copy/Paste" tool
 */

class FLandscapeEditorDetailCustomization_CopyPaste : public FLandscapeEditorDetailCustomization_Base
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) OVERRIDE;

public:
	static FReply OnCopyToGizmoButtonClicked();
	static FReply OnFitGizmoToSelectionButtonClicked();
	static FReply OnFitHeightsToGizmoButtonClicked();
	static FReply OnClearGizmoDataButtonClicked();
	static FReply OnGizmoHeightmapFilenameButtonClicked(TSharedRef<IPropertyHandle> HeightmapPropertyHandle);

	bool GetGizmoImportButtonIsEnabled() const;
	FReply OnGizmoImportButtonClicked();
	FReply OnGizmoExportButtonClicked();
};

class FLandscapeEditorStructCustomization_FGizmoImportLayer : public FLandscapeEditorStructCustomization_Base
{
public:
	static TSharedRef<IStructCustomization> MakeInstance();

	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;

public:
	static FReply OnGizmoImportLayerFilenameButtonClicked(TSharedRef<IPropertyHandle> PropertyHandle_LayerFilename);
};
