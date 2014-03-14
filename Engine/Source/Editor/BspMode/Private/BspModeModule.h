// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IBspModeModule.h"

struct FBspBuilderType
{
	FBspBuilderType(class UClass* InBuilderClass, const FText& InText, const FText& InToolTipText, const FSlateBrush* InIcon)
		: BuilderClass(InBuilderClass)
		, Text(InText)
		, ToolTipText(InToolTipText)
		, Icon(InIcon)
	{
	}

	/** The class of the builder brush */
	TWeakObjectPtr<UClass> BuilderClass;

	/** The name to be displayed for this builder */
	FText Text;

	/** The name to be displayed for this builder */
	FText ToolTipText;

	/** The icon to be displayed for this builder */
	const FSlateBrush* Icon;
};

class FBspModeModule : public IBspModeModule
{
public:
	
	/** IModuleInterface interface */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** IBspModeModule interface */
	virtual TSharedRef< SWidget > CreateBspModeWidget() const OVERRIDE;
	virtual TSharedRef< FEdMode > GetBspMode() const OVERRIDE;
	virtual void RegisterBspBuilderType( class UClass* InBuilderClass, const FText& InBuilderName, const FText& InBuilderTooltip, const FSlateBrush* InBuilderIcon ) OVERRIDE;
	virtual void UnregisterBspBuilderType( class UClass* InBuilderClass ) OVERRIDE;

	const TArray< TSharedPtr<FBspBuilderType> >& GetBspBuilderTypes();
	TSharedPtr<FBspBuilderType> FindBspBuilderType(UClass* InBuilderClass) const;

private:

	TSharedPtr<class FBspMode> BspMode;

	TArray< TSharedPtr<FBspBuilderType> > BspBuilderTypes;
};
