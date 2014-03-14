// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceProfileDetails.h: Declares the FDeviceProfileDetails class.
=============================================================================*/

#pragma once

/**
 * Implements details panel customizations for UDeviceProfile fields.
 */
class FDeviceProfileDetails
	: public IDetailCustomization
{
public:

	// Begin IDetailCustomization interface
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;
	// End IDetailCustomization interface


public:

	/**
	 * Makes a new instance of this detail layout class.
	 *
	 * @return The created instance.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FDeviceProfileDetails);
	}


protected:
};
