// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class EDITORWIDGETS_API SCreateNewAssetFromFactory : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateNewAssetFromFactory)
	{}

	SLATE_END_ARGS()

	virtual ~SCreateNewAssetFromFactory() {}

	DEPRECATED(4.8, "Please do not use SCreateNewAssetFromFactory::Create - use IAssetTools::CreateAsset instead.")
	static UObject* Create(TWeakObjectPtr<UFactory> FactoryPtr);

	void Construct(const FArguments& InArgs, TSharedPtr<SWindow> InParentWindow, TWeakObjectPtr<UFactory> InFactoryPtr) {}
};
