// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDocumentationActorDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	
	/* Handler for clicking the help button */
	FReply OnHelpButtonClicked();
private:
	/* The documentation actor that we are showing in the details panel */
	TWeakObjectPtr<class ADocumentationActor>	SelectedDocActor;
};
