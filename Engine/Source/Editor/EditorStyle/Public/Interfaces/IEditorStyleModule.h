// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


class IEditorStyleModule : public IModuleInterface
{
public:

	virtual TSharedRef< class FSlateStyleSet > CreateEditorStyleInstance() const = 0;
};
