#pragma once 

#if ARKIT_SUPPORT

// ARKit
#include <ARKit/ARKit.h>

@interface FAppleARKitSessionDelegate : NSObject<ARSessionDelegate>
{
}

- (id)initWithAppleARKitSession:(class UAppleARKitSession*)InAppleARKitSession;

- (void)setMetalTextureCache:(CVMetalTextureCacheRef)InMetalTextureCache;

@end

#endif // ARKIT_SUPPORT
