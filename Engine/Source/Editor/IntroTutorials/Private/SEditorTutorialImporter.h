// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SEditorTutorialImporter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SEditorTutorialImporter ) {}

	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FText GetImportPath() const;

	void Import(UEditorTutorial* InTutorial);

private:

	void OnImport();

	void OnCancel();

	void HandleTextCommitted(const FText& InText, ETextCommit::Type InCommitType);

	FReply OnPickFile();

private:
	/** The path we will be importing from */
	FText UDNPath;

	/** Parent window of this widget */
	TWeakPtr<SWindow> ParentWindow;
};
