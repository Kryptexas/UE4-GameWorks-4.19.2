/************************************************************************************

Filename    :   OVRVersion.h
Content     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#ifndef _OVR_VERSION_H
#define _OVR_VERSION_H

// At some point we will transition to product version 1 
// and reset the major version back to 1 (first product release, version 1.0).
#define OVR_PRODUCT_VERSION 0
#define OVR_MAJOR_VERSION	6
#define OVR_MINOR_VERSION	0
#define OVR_PATCH_VERSION	1
#define OVR_BUILD_VERSION	0

/// "Product.Major.Minor.Patch"
// Note: this is an array rather than a char* because this makes it
// more convenient to extract the value of the symbol from the binary
extern const char OVR_VERSION_STRING[];

#endif
