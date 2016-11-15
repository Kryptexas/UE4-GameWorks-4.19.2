// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Interface implemented by objects that hold a persona toolkit */
class IHasPersonaToolkit
{
public:
	/** Get the toolkit held by this object */
	virtual TSharedRef<class IPersonaToolkit> GetPersonaToolkit() const = 0;
};