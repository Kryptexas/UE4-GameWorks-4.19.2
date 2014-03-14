// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class ISequencerSection;
class FTrackNode;

class FSectionLayoutBuilder : public ISectionLayoutBuilder
{
public:
	FSectionLayoutBuilder( TSharedRef<FTrackNode> InRootNode );

	/** ISectionLayoutBuilder Interface */
	virtual void PushCategory( FName CategoryName, const FString& DisplayLabel ) OVERRIDE;
	virtual void SetSectionAsKeyArea( TSharedRef<IKeyArea> KeyArea ) OVERRIDE;
	virtual void AddKeyArea( FName KeyAreaName, const FString& DisplayName, TSharedRef<IKeyArea> KeyArea ) OVERRIDE;
	virtual void PopCategory() OVERRIDE;
private:
	/** Root node of the tree */
	TSharedRef<FTrackNode> RootNode;
	/** The current node that other nodes are added to */
	TSharedRef<FSequencerDisplayNode> CurrentNode;
};
