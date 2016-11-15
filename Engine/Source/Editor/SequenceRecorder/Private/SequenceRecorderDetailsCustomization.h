// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSequenceRecorderDetailsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FSequenceRecorderDetailsCustomization);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
