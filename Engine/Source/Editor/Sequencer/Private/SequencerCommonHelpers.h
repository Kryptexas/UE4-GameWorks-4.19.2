// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class SequencerHelpers
{
public:
	/**
	 * Gets the key areas from the requested node
	 */
	static void GetAllKeyAreas(TSharedPtr<FSequencerDisplayNode> DisplayNode, TSet<TSharedPtr<IKeyArea>>& KeyAreas);

	/**
	* Get descendant nodes
	*/
	static void GetDescendantNodes(TSharedRef<FSequencerDisplayNode> DisplayNode, TSet<TSharedRef<FSequencerDisplayNode> >& Nodes);
};