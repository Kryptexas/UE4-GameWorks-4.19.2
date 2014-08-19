// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * The public interface implemented by the UMG Designer to allow extensions to call methods
 * on the designer.
 */
class UMGEDITOR_API IUMGDesigner
{
public:
	/** @return the effective preview scale after both the DPI and Zoom scale has been applied. */
	virtual float GetPreviewScale() const = 0;
};
