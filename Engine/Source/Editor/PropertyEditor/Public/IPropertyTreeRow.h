// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class IPropertyTreeRow
{
public: 

	virtual TSharedPtr< FPropertyPath > GetPropertyPath() const = 0;

	virtual bool IsCursorHovering() const = 0;
};