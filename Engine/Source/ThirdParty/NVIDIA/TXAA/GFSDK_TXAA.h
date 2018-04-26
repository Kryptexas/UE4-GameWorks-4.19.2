// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.

 
/*===========================================================================
GFSDK_TXAA.h
=============================================================================

NVIDIA TXAA v3.0

-----------------------------------------------------------------------------
ENGINEERING CONTACT
-----------------------------------------------------------------------------
For any feedback or questions about this library, please contact devsupport@nvidia.com.

-----------------------------------------------------------------------------
FEATURES
-----------------------------------------------------------------------------
* Core TXAA modes
* Softer film style resolve + reduction of temporal aliasing. Starting from this release, 
  one can optionally choose to use a wider Blackman-Harris3.0 filter instead of box filter.
* Temporal super-sampling by jittering the projection matrix.
* Anti-flicker filter that reduces the flicker in temporal super-sampling mode when there is no motion.
* Tighter color-space bounds based on the statistical metrics that uses the algorithm developed by NVIDIA research.

TEMPORAL REPROJECTION MODES

* Motion vectors explicitly passed
* Application must generate motion vectors, so potentially more work to integrate
* Ghosting can be eliminated by accounting for object motion and camera motion

MSAA MODES
* 1xMSAA or Non-MSAA
* 2xMSAA
* 4xMSAA
* 8xMSAA

-----------------------------------------------------------------------------
DEBUG CHANNELS
-----------------------------------------------------------------------------
The TXAA resolve takes a debug render target that can aid debugging.

* VIEW MSAA
* Standard MSAA box filter resolve. Optionally Blackman-Harris3.0 filter can be used.
* Optional Anti-flicker filter that reduces temporal flickering (controlled by check-box).
* Mode: NV_TXAA_DEBUGCHANNEL_MSAA

VIEW MOTION VECTORS

* Coloring,
    * Light Blue-Purple -> no movement
    * Pink -> objects moving left
    * Blue -> objects moving up
* Mode: NV_TXAA_DEBUGCHANNEL_MOTIONVECTORS

VIEW DIFFERENCE BETWEEN MSAA AND TEMPORAL REPROJECTION
* Shows difference of current frame and reprojected prior frame
* Current MSAA frame is box filtered
* Used to check if input view projection matrixes are correct
* Should see convergance to a black screen on a static image.
* Should see an outline on edges in motion, and the outline should
not grow in size under motion.
* Should see bright areas representing occlusion regions (where the
content is only visible in one frame).
* Should see bright areas when motion exceeds the input limits.
Note: the input limits are on a curve.
* Mode: NV_TXAA_DEBUGCHANNEL_TEMPORALDIFFS

VIEW TEMPORAL REPROJECTION
* Shows just the temporally reprojected prior frame
* Mode: NV_TXAA_DEBUGCHANNEL_TEMPORALREPROJ

DEBUG VIEW CONTROL
* Shows the texture used to control the filter softness and the amount
of temporal feedback
* Channel: NV_TXAA_DEBUGCHANNEL_CONTROL

-----------------------------------------------------------------------------
PIPELINE
-----------------------------------------------------------------------------
TXAA Replaces the MSAA resolve step with this custom resolve pipeline

AxB sized MSAA RGBA color   --------->  T  --> AxB sized RGBA resolved color
AxB sized motion vector     --------->  X                           |
AxB sized RGBA feedback     --------->  A                           |
AxB sized depth information --------->  A                           |
|                                                                |
+----------------------------------------------------------------+
TXAA needs two AxB sized resolve surfaces (because of the feedback).

NOTE, if the AxB sized RGBA resolved color surface is going to be modified after 
TXAA, then it needs to be copied, and the unmodified copy 
needs to be feed back into the next TXAA frame.

NOTE, resolved depth is best resolved for TXAA with the minimum depth of all 
samples in the pixel. If required one can write min depth for TXAA and max depth 
for particles in the same resolve pass (if the game needs max depth for something else).
If the TXAA algorithm is invoked with multiple samples, one can pass in unresolved
multi-sampled depth as an input to TXAA, especially for the mode that choses 
the motion vector for the closest sample.

Starting with this release, users can apply jitter offsets of less than one pixel 
to the projection matrix. This has the same effect as that of moving the visibility 
and shading samples by the same offsets in the opposite directions. The offsets 
can be from a low discrepancy sequence e.g. Sobol, and repeat after some number 
of frames e.g. 16. During the spatial resolve phase, we need to filter the results 
and filter weights must be adjusted for these offsets. Jitter amounts are conveyed
to the API every frame.

-----------------------------------------------------------------------------
SLI SCALING
-----------------------------------------------------------------------------
TXAA is a temporal approach to antialiasing; As such, it introduces an
interframe dependency on the RGBA feedback surface, which could hinder SLI
scaling in AFR mode. An approach for removing the interframe dependency is to
use the NvAPI to manage a separate feedback buffer for each GPU. The TXAA11
sample application that is included with the TXAA library uses the technique
to remove the interframe dependency, improving SLI scaling.

-----------------------------------------------------------------------------
USE LINEAR
-----------------------------------------------------------------------------
TXAA requires linear RGB color input.
* Use DXGI_FORMAT_*_SRGB for 8-bit/channel colors (LDR); or
* Make sure sRGB->linear (TEX) and linear->sRGB (ROP) conversion is on.
* Use DXGI_FORMAT_R16G16B16A16_FLOAT for HDR.

Tone-mapping the linear HDR data to low-dynamic-range is done in TXAA and TXAA
will preserve the linear color space when finishes its pass.
For example, after post-processing at the time of color-grading.
The hack to tone-map prior to resolve, to workaround the artifacts
of the traditional MSAA box filter resolve, is not needed with TXAA.
Tone-map prior to resolve will result on wrong colors on thin features.

There is support for compressing some or all of the dynamic range before
the resolve using the NvTxaaCompressionRange struct. With TXAA dynamic range
compression enabled, the resolved frame will be decompressed back to linear
HDR automatically; the application should always supply linear HDR data, and
should expect linear HDR results back from TXAA.

-----------------------------------------------------------------------------
THE MOTION VECTOR FIELD
-----------------------------------------------------------------------------
The motion vector field input is FP16, where:
* RG provides a vector {x,y} offset to the location of the pixel
in the prior frame.
* The offset is scaled such that {1,0} is a X shift the size of the frame.
Or {0,1} is a Y shift the size of the frame.

-----------------------------------------------------------------------------
GPU API STATE
-----------------------------------------------------------------------------
This library does not save and restore GPU state.
All modified GPU state is documented below by the function prototypes.

------------------------------------------------------------------------------

-----------------------------------------------------------------------------
INTEGRATION EXAMPLE
-----------------------------------------------------------------------------
(0.) INITIALIZE CONTEXT

// DX11
#define __GFSDK_DX11__
#include <GFSDK_TXAA.h>
NvTxaaContextDX11 txaaCtx;
ID3D11Device *dev;
if (NV_TXAA_STATUS_OK != GFSDK_TXAA_DX11_InitializeContext(&txaaCtx, dev)) // No TXAA.

// GL
#define __GFSDK_GL__
#include <GFSDK_TXAA.h>
TxaaCtx txaaCtx;
if (NV_TXAA_STATUS_OK != GFSDK_TXAA_GL_InitializeContext(&txaaCtx)) // No TXAA.

Replace the MSAA Resolve Step with the TXAA Resolve


(1.) REPLACE THE MSAA RESOLVE STEP WITH THE TXAA RESOLVE

Custom Motion Vector

// DX11
NvTxaaContextDX11 *ctx;             // TXAA context.
ID3D11DeviceContext *dxCtx;         // DX11 device context.
ID3D11RenderTargetView *dstRtv;     // Destination texture.
ID3D11ShaderResourceView *msaaSrv;  // Source MSAA texture shader resource view.
ID3D11ShaderResourceView *msaaDepth;// Source MSAA texture shader resource view.
ID3D11ShaderResourceView *mvSrv;    // Source motion vector texture SRV.
ID3D11ShaderResourceView *dstSrv;   // SRV to feedback resolve results from prior frame.

NvTxaaResolveParametersDX11 resolveParameters;
memset(&resolveParameters, 0, sizeof(resolveParameters));
resolveParameters.txaaContext = &m_txaaContext;
resolveParameters.deviceContext = deviceContext;
resolveParameters.resolveTarget = m_resolveTarget.pRTV;
resolveParameters.msaaSource = srv;
resolveParameters.msaaDepth = (m_presetMode == PRESETMODE_0xTXAA) ? m_TXAASceneDepthResolved.pSRV : m_TXAASceneDepth.pSRV;
resolveParameters.feedbackSource = feedback.m_txaaFeedback.pSRV;
resolveParameters.alphaResolveMode = NV_TXAA_ALPHARESOLVEMODE_RESOLVESRCALPHA;
resolveParameters.feedback = &m_feedback;
m_txaaPerFrameConstants.xJitter = pJitter[0];
m_txaaPerFrameConstants.yJitter = pJitter[1];
m_txaaPerFrameConstants.mvScale = m_MVScale;
m_txaaPerFrameConstants.motionVecSelection = NV_TXAA_USEMV_NEAREST;
m_txaaPerFrameConstants.useRGB = 0;
m_txaaPerFrameConstants.frameBlendFactor = GetBlendFactor();
m_txaaPerFrameConstants.dbg1 = GetDbg1();
m_txaaPerFrameConstants.bbScale = GetBBScale();
m_txaaPerFrameConstants.enableClipping = GetEnableClipping();

// GL
NvTxaaContextGL *ctx;             // TXAA context.
GLuint *dst;                      // Destination texture.
GLuint msaaSrc;                   // Source MSAA texture.
GLUint mv;                        // Source motion vector texture.
gfsdk_U32 frameCounter;           // Increment every frame, notice the ping pong of dst.
NvTxaaResolveParametersGL resolveParameters;
memset(&resolveParameters, 0, sizeof(resolveParameters));
resolveParameters.txaaContext = ctx;
resolveParameters.resolveTarget = dst[(frameCounter + 1) & 1;
resolveParameters.msaaSource = msaaSrc;
resolveParameters.feedback = dst[frameCounter & 1];
resolveParameters.alphaResolveMode = NV_TXAA_ALPHARESOLVEMODE_RESOLVESRCALPHA;
GFSDK_TXAA_GL_ResolveFromMotionVectors(&resolveParameters, mv);

(3.) RELEASE CONTEXT

// DX11
#define __GFSDK_DX11__
#include <GFSDK_TXAA.h>
NvTxaaContextDX11 txaaCtx;
GFSDK_TXAA_DX11_ReleaseContext(&txaaCtx);

// GL
#define __GFSDK_GL__
#include <GFSDK_TXAA.h>
NvTxaaContextGL txaaCtx;
GFSDK_TXAA_GL_ReleaseContext(&txaaCtx);

-----------------------------------------------------------------------------
INTEGRATION SUGGESTIONS
-----------------------------------------------------------------------------
(1.) Get the debug pass-through modes working,
for instance NV_TXAA_DEBUGCHANNEL_MSAA.

(2.) Get the TXAA resolve working on still frames.

(2.a.) Verify motion vector field sign is correct
using NV_TXAA_DEBUGCHANNEL_MOTIONVECTORS.
Output should be,
Light Blue-Purple	-> no movement
Pink				-> objects moving left
Blue				-> objects moving up
If this is wrong, try the transpose of mvpCurrent and mvpPrior.

(2.b.) Verify motion vector field scale is correct
using NV_TXAA_DEBUGCHANNEL_TEMPORALDIFFS.
Should see,
No    Motion -> edges
Small Motion -> edges maintaining brightness and thickness
If seeing edges increase in size with a simple camera rotation,
then motion vector input or mvpCurrent and/or mvpPrior is incorrect.

(2.c.) Once the motion field is verified, the TXAA resolve should work with motion

-----------------------------------------------------------------------------
MATRIX CONVENTION FOR INPUT
-----------------------------------------------------------------------------
Given a matrix,

  mxx mxy mxz mxw
  myx myy myz myw
  mzx mzy mzz mzw
  mwx mwy mwz mww

A matrix vector multiply is defined as,

  x' = x*mxx + y*mxy + z*mxz + w*mxw
  y' = x*myx + y*myy + z*myz + w*myw
  z' = x*mzx + y*mzy + z*mzz + w*mzw
  w' = x*mwx + y*mwy + z*mwz + w*mww
         
/////////////////////////////////////////////////////////////////////////////
===========================================================================*/
#if !defined(__GFSDK_TXAA_H__)
#	define __GFSDK_TXAA_H__

/////////////////////////////////////////////////////////////////////////////
/// @file GFSDK_TXAA.h
/// @brief The TXAA API C/C++ header
/////////////////////////////////////////////////////////////////////////////

/*===========================================================================
INCLUDES 
===========================================================================*/

// defines general GFSDK includes and structs
#include "GFSDK_TXAA_Common.h"

#if defined(__GFSDK_OS_WINDOWS__)
#	include <windows.h>
#endif

#if defined(__GFSDK_DX11__)
#	include <d3d11.h>
#endif

#if defined(__GFSDK_GL__)
#   if defined(__GFSDK_OS_ANDROID__)
#       include <Regal/GL/Regal.h>
#   else
#	    include <GL3/gl3.h>
#   endif
#endif

#include <float.h>

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains the library version number.
////////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaVersion
{
	gfsdk_U32 major;    //!< Major version of the product, changed manually with every product release with a large new feature set.
	gfsdk_U32 minor;    //!< Minor version of the product, changed manually with every minor product release containing some features.
	gfsdk_U32 revision; //!< Revision, changed manually with every revision for bug fixes.
	gfsdk_U32 build;    //!< Build number of the same revision. Typically 0.
} NvTxaaVersion;

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains the current library version number.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaVersion NvTxaaCurrentVersion = { 3, 0, 0, 0 };

/////////////////////////////////////////////////////////////////////////////
/// @brief Status codes that may be returned by functions in the library.
/////////////////////////////////////////////////////////////////////////////
typedef enum NvTxaaStatus
{
	// General status
	NV_TXAA_STATUS_OK                           =  0,   //!< Success. Request is completed.
	NV_TXAA_STATUS_ERROR                        = -1,   //!< Generic error. Request failed.
	NV_TXAA_STATUS_SDK_VERSION_MISMATCH         = -2,   //!< Mismatch of dll vs header file+lib
	NV_TXAA_STATUS_UNSUPPORTED                  = -3,   //!< The request is unsupported
	NV_TXAA_STATUS_INVALID_ARGUMENT             = -4,   //!< Invalid argument (such as a null pointer)
	
	// Object validation
	NV_TXAA_STATUS_CONTEXT_NOT_INITIALIZED      = -100, //!< The supplied TXAA context is not initialized
	NV_TXAA_STATUS_CONTEXT_ALREADY_INITIALIZED  = -101, //!< The supplied TXAA context was already initialized

	// Resolve argument validation
	NV_TXAA_STATUS_INVALID_DEVICECONTEXT        = -200, //!< The supplied device context is invalid (doesn't belong to the device that the TXAA context was initialized with)
	NV_TXAA_STATUS_INVALID_RESOLVETARGET        = -201, //!< The supplied resolve target is invalid (the wrong format or dimensions)
	NV_TXAA_STATUS_INVALID_MSAASOURCE           = -202, //!< The supplied MSAA source is invalid (the wrong format or dimensions)
	NV_TXAA_STATUS_INVALID_FEEDBACKSOURCE       = -203, //!< The supplied feedback source is invalid (the wrong format or dimensions)
	NV_TXAA_STATUS_INVALID_MOTIONVECTORS        = -204, //!< The supplied motion vector field is invalid (the wrong format or dimensions)
	NV_TXAA_STATUS_INVALID_CONTROLSOURCE        = -205, //!< The supplied control source is invalid (the wrong format or dimensions)
	NV_TXAA_STATUS_INVALID_COMPRESSIONRANGE     = -206, //!< The supplied compression range is invalid
    NV_TXAA_STATUS_INVALID_DEPTH                = -207, //!< The supplied depth source is invalid (the wrong format or dimensions)
} NvTxaaStatus;

/////////////////////////////////////////////////////////////////////////////
/// @brief Alpha resolve mode.
///
/// Controls what value is put into the destination alpha channel in the
/// TXAA resolve operation.
/// This enumeration affects nothing in alpha calculation. It will be 
/// removed.
/////////////////////////////////////////////////////////////////////////////
typedef enum NvTxaaAlphaResolveMode
{
	NV_TXAA_ALPHARESOLVEMODE_KEEPDESTALPHA = 0, //!< Keeps the desitnation alpha channel value
    NV_TXAA_ALPHARESOLVEMODE_RESOLVESRCALPHA,   //!< Dest alhpa gets a box filtered version of the source MSAA alpha channel
	NV_TXAA_ALPHARESOLVEMODE_COUNT
} NvTxaaAlphaResolveMode;

/////////////////////////////////////////////////////////////////////////////
/// @brief Dynamic range compression data.
///
/// Dynamic range ompression occurs before the reconstruction filter and
/// temporal feedback is applied in the TXAA resolve. The final resolved
/// color is expanded back to the original dynamic range before being
/// written to the TXAA resolve target.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaCompressionRange
{
	gfsdk_F32 threshold; //!< Colors with luma above this range will be compressed, below will be untouched
	gfsdk_F32 range;     //!< The range of compression (should be >= threshold)
} NvTxaaCompressionRange;

/////////////////////////////////////////////////////////////////////////////
/// @brief Sample offsets that will be used in filtering.
///
/// The jitter offsets that are applied to the viewport matrix decide the filter
/// weights that get used. Filter weights depend on the sample offsets. These 
/// offsets are same across all the pixels because same amount of jitter is applied
/// to the entire viewport. They are laid out for each multi-sampling mode one 
/// after another. For a given MSAA mode and a frame, filter tap weights are laid 
/// out from 0..8 starting at the top left pixel and going row-wise to the bottom 
/// right pixel.
/// 
/////////////////////////////////////////////////////////////////////////////
#define NV_TXAA_MAX_NUM_FRAMES              16
#define NV_TXAA_FILTER_TAPS_BLACKMAN_HARRIS 9 
#define NV_TXAA_FILTER_SIZE                 4
#define NV_TXAA_NUM_MAX_SAMPLES             8

typedef enum NvTxaaUseMV
{
    NV_TXAA_USEMV_LONGEST_LOCAL,         //!< Use longest MV within a pixel
    NV_TXAA_USEMV_LONGEST_NEIGHBORHOOD,  //!< Use longest MV within a + shaped neighborhood
    NV_TXAA_USEMV_BEST_FIT,              //!< Use two MVs longest and shortest, and use the one that produces the closest color from the history
    NV_TXAA_USEMV_NEAREST,               //!< Use the MV of the sample that is closest
    NV_TXAA_USEMV_COUNT
} NvTxaaUseMV;

/////////////////////////////////////////////////////////////////////////////
/// @brief UI Constants that need to be passed to the shaders
///
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaPerFrameConstants
{
    gfsdk_U32       useBHFilters;         //!< Use Blackman Harris 3.3 filter
    gfsdk_U32       useAntiFlickerFilter; //!< Use Anti flicker filter
    gfsdk_U32       motionVecSelection;  //!< Use longest MV in the neighborhood
    gfsdk_U32       useRGB;               //!< Use RGB instead of YCoCg

    gfsdk_U32       isZFlipped;           //!< Is Z flipped
    gfsdk_F32       xJitter;              //!< Jitter along X-axis
    gfsdk_F32       yJitter;              //!< Jitter along Y-axis
    gfsdk_F32       mvScale;              //!< Motion Vector Scale

    gfsdk_U32       enableClipping;     //!< Enable clipping
    gfsdk_F32       frameBlendFactor;   //!< frame blend factor
    gfsdk_U32       dbg1;               //!< dbg variable
    gfsdk_F32       bbScale;            //!< Scale factor to inflate bounding box
    
    NvTxaaPerFrameConstants() : useBHFilters(1), useAntiFlickerFilter(0),
        motionVecSelection(NV_TXAA_USEMV_NEAREST), useRGB(false),
        isZFlipped(false), xJitter(0.f), yJitter(0.f), mvScale(1024.f),
        enableClipping(1), frameBlendFactor(0.04f), dbg1(false), bbScale(1.f) {};
} NvTxaaPerFrameConstants;


////////////////////////////////////////////////////////////////////////////////
/// @brief Contains range compression settings for no compression.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaCompressionRange NvTxaaNoCompression = { 0.0f , 1.0f };

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains range compression settings for full compression.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaCompressionRange NvTxaaFullCompression = { 0.0f, 1.0f };

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains range compression settings for half compression.
///
/// Only the colors in the upper half of standard luma range and HDR colors
/// are compressed.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaCompressionRange NvTxaaHalfCompression = { 0.5f, 1.0f };

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains range compression settings for HDR compression.
///
/// Only colors that exceed the standard (0-1) luma range are commpressed.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaCompressionRange NvTxaaHdrCompression = { 1.0f, 2.0f };

////////////////////////////////////////////////////////////////////////////////
/// @brief Default range compression settings.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaCompressionRange NvTxaaDefaultCompression = NvTxaaNoCompression;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Feedback Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaFeedbackParameters
{	
	/// <summary>
	/// Weight to apply to feedback that is not clipped.
	/// </summary>
	gfsdk_F32 weight;
	
	/// <summary>
	/// Weight to apply to feedback that is clipped.
	/// </summary>
	gfsdk_F32 clippedWeight;
} NvTxaaFeedbackParameters;

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains default feedback parameters.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaFeedbackParameters NvTxaaDefaultFeedback = { 0.75f, 0.75f };

////////////////////////////////////////////////////////////////////////////////
/// @brief Motion parameters
////////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaMotionParameters
{	
	/// <summary>
	/// The x motion offset (0-1, same scale as motion vectors).
	/// The motion offset is added to the motion from the velocity field.
	/// </summary>
	gfsdk_F32 motionOffsetX;

	/// <summary>
	/// The y motion offset (0-1, same scale as motion vectors).
	/// The motion offset is added to the motion from the velocity field.
	/// </summary>
	gfsdk_F32 motionOffsetY;
	
	/// <summary>
	/// The motion limit in pixels; 16 pixels is recommended
	/// </summary>
	gfsdk_F32 motionLimitPixels;	
} NvTxaaMotionParameters;

////////////////////////////////////////////////////////////////////////////////
/// @brief Contains default motion parameters.
////////////////////////////////////////////////////////////////////////////////
static const NvTxaaMotionParameters NvTxaaDefaultMotion = { 0.0f, 0.0f, 16.0f };

/////////////////////////////////////////////////////////////////////////////
/// @brief Debug channel
/////////////////////////////////////////////////////////////////////////////
typedef enum NvTxaaDebugChannel
{
	NV_TXAA_DEBUGCHANNEL_MSAA,           //!< Render MSAA resolve
	NV_TXAA_DEBUGCHANNEL_MOTIONVECTORS,  //!< Render motion vectors
	NV_TXAA_DEBUGCHANNEL_TEMPORALDIFFS,  //!< Render temporal differences
	NV_TXAA_DEBUGCHANNEL_TEMPORALREPROJ, //!< Render the temporally reprojected image
	NV_TXAA_DEBUGCHANNEL_CONTROL,        //!< Render the AA control values
    NV_TXAA_DEBUGCHANNEL_COUNT
} NvTxaaDebugChannel;

/*===========================================================================
FUNCTIONS
===========================================================================*/
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__TXAA_CPP__)
#	define TXAA_API __GFSDK_EXPORT__
#else
#	define TXAA_API __GFSDK_IMPORT__
#endif

/*===========================================================================
TXAA DIRECTX 11 API FUNCTIONS
===========================================================================*/
#if defined(__GFSDK_DX11__)

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Context.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaContextDX11 NvTxaaContextDX11;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Debug Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaDebugParametersDX11
{
	/// <summary>
	/// Debug render target, valid if it:
	///   - Has the same dimensions (width and height) as msaaSource.
	///   - Is a view of a 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	ID3D11RenderTargetView   *target;
	
	/// <summary>
	/// Specifies what should be rendered into debugTarget
	/// </summary>
	NvTxaaDebugChannel        channel;
} NvTxaaDebugParametersDX11;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Resolve Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaResolveParametersDX11
{	
	/// <summary>
	/// The TXAA context
	/// </summary>
	NvTxaaContextDX11         *txaaContext;
	
	/// <summary>
	/// The device context
	/// </summary>
	ID3D11DeviceContext       *deviceContext;

	/// <summary>
	/// Target of the resolve; valid if it:
	///   - Is a view of a 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	ID3D11RenderTargetView    *resolveTarget;

	/// <summary>
	/// Source of the resolve; valid if it:
	///   - Has the same dimensions (width and height) as resolveTarget.
	///   - Is a view of a Non-MSAA, 1X, 2X, 4X or 8X MSAA 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	ID3D11ShaderResourceView  *msaaSource;

    /// <summary>
    /// Source Depth of the resolve; valid if it:
    ///   - Has the same dimensions (width and height) as resolveTarget.
    ///   - Is a view of a Non-MSAA, 1X, 2X, 4X or 8X MSAA 2D depth-stencil buffer.
    ///   - Was created by the device that owns deviceContext.
    /// </summary>
    ID3D11ShaderResourceView  *msaaDepth;


    /// <summary>
	/// Feedback from the prior frame (prior frame's resolveTarget); valid if it:
	///   - Has the same dimensions (width and height) as resolveTarget.
	///   - Is a view of a 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	ID3D11ShaderResourceView  *feedbackSource;
	
	/// <summary>
	/// Texture used for per-pixel control of TXAA application; may be null, valid if it:
	///   - Is a view of a 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	ID3D11ShaderResourceView  *controlSource;
	
	/// <summary>
	/// The alpha resolve mode.
	/// </summary>
	NvTxaaAlphaResolveMode     alphaResolveMode;
		
	/// <summary>
	/// The compression range to use for resolve; null to disable compression.
	/// </summary>
	NvTxaaCompressionRange    *compressionRange;

	/// <summary>
	/// The feedback parameters to use for resolve; null for default feedback parameters.
	/// </summary>
	NvTxaaFeedbackParameters  *feedback;

	/// <summary>
	/// The debug parameters to use for resolve; may be null.
	/// </summary>
	NvTxaaDebugParametersDX11 *debug;

    /// <summary>
    /// UI controls
    /// </summary>
    NvTxaaPerFrameConstants *perFrameConstants;

} NvTxaaResolveParametersDX11;


/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Motion Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaMotionDX11
{			
	/// <summary>
	/// Motion vector field; valid if it:
	///	- Has the same dimensions (width and height) as resolveTarget.
	///	- Is a view of a 2D texture.
	/// - Was created by the device that owns deviceContext.
	/// </summary>	
	ID3D11ShaderResourceView *motionVectors;
    /// <summary>
    /// Unresolved motion vector field; valid if it:
    ///	- Has the same dimensions (width and height) as resolveTarget.
    ///   Has the same number of samples.
    ///	- Is a view of a 2D texture.
    /// - Was created by the device that owns deviceContext.
    /// </summary>	
    ID3D11ShaderResourceView *motionVectorsMS;
    /// <summary>
	/// The motion parameters; null to use default motion parameters.
	/// </summary>
	NvTxaaMotionParameters   *parameters;
} NvTxaaMotionDX11;

/////////////////////////////////////////////////////////////////////////////
/// @brief Initializes the TXAA library for the specified context.
///
/// Note this might be a high latency operation as it does a GPU->CPU read.
///    - This is Metro safe.
///    - Might want to spawn a thread to do this async from normal load.
///
/// @param[in] txaaContext Pointer to an uninitialized NvTxaaContextDX11
///            structure to be initialized
///
/// @param[in] device DirectX device
///
/// @param[in] version Pointer to an NvTxaaVersion structure; the application
///            should pass the address of the NvTxaaCurrentVersion constant
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if txaaContext or device is null;
///          NV_TXAA_STATUS_SDK_VERSION_MISMATCH if the application was compiled
///          against an incompatible version of the library;
///          NV_TXAA_STATUS_CONTEXT_ALREADY_INITIALIZED if txaaContext has
///          already been initialized;
///          NV_TXAA_STATUS_VERSION_MISMATCH if the application was compiled
///          against an incompatible version of the library;
///          NV_TXAA_STATUS_UNSUPPORTED if TXAA is unsuppported;
///          NV_TXAA_STATUS_OK if initialization succeeded
///
/// @sideeffect The following methods will be called on the supplied
/// device's immediate context:
///    - OMSetRenderTargets()
///    - PSSetShaderResources()
///    - RSSetViewports()
///    - VSSetShader()
///    - PSSetShader()
///    - PSSetSamplers()
///    - VSSetConstantBuffers()
///    - PSSetConstantBuffers()
///    - RSSetState()
///    - OMSetDepthStencilState()
///    - OMSetBlendState()
///    - IASetInputLayout()
///    - IASetPrimitiveTopology()
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_DX11_InitializeContext(
	NvTxaaContextDX11   *txaaContext,
	ID3D11Device        *device,
	const NvTxaaVersion *version = &NvTxaaCurrentVersion);

/////////////////////////////////////////////////////////////////////////////
/// @brief Releases the specified TXAA context.
///
/// @param[in] txaaContext Pointer to an initialized NvTxaaContextDX11
///            structure to be released
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if txaaContext is null;
///          NV_TXAA_STATUS_CONTEXT_NOT_INITIALIZED if txaaContext has not
///          been initialized with a call to GFSDK_TXAA_DX11_InitializeContext;
///          NV_TXAA_STATUS_OK if the context has been fully released
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_DX11_ReleaseContext(
	NvTxaaContextDX11 *txaaContext);

/////////////////////////////////////////////////////////////////////////////
/// @brief Resolves the supplied mutisample surface using a supplied motion
/// vector field. If non-MSAA surface is provided, this will triger a
/// non-MSAA mode TXAA resolve.
/// 
/// Use in-place of standard MSAA resolve.
///
/// The supplied control texture can be used to control the amount of
/// antialiasing [0-1]. The filter is narrowed and temporal feedback is
/// limited in areas with lower control values. If not supplied, all pixels
/// use the widest filter and maximum temporal feedback.
///
/// @param[in] resolveParameters Resolve parameters
///
/// @param[in] motion Motion data
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if resolveParameters or motionVectors is null,
///          or if any required resolve parameter is null;
///          NV_TXAA_STATUS_CONTEXT_NOT_INITIALIZED if resolveParameters->txaaContext
///          has not been initialized with a call to GFSDK_TXAA_DX11_InitializeContext;
///          NV_TXAA_STATUS_INVALID_DEVICECONTEXT if resolveParameters->deviceContext
///          doesn't belong to the device that txaaContext was initialized with;
///          NV_TXAA_STATUS_INVALID_RESOLVETARGET if resolveParameters->resolveTarget is invalid;
///          NV_TXAA_STATUS_INVALID_MSAASOURCE if resolveParameters->msaaSource is invalid;
///          NV_TXAA_STATUS_INVALID_FEEDBACKSOURCE if resolveParameters->feedback is invalid;
///          NV_TXAA_STATUS_INVALID_CONTROLSOURCE if resolveParameters->controlSource is invalid;
///          NV_TXAA_STATUS_INVALID_COMPRESSIONRANGE if resolveParameters->compressionRange is invalid;
///          NV_TXAA_STATUS_INVALID_MOTIONVECTORS if motion->motionVectors is invalid;
///          NV_TXAA_STATUS_OK if the resolve succeeded
///
/// @sideeffect The following methods will be called on the deviceContext:
///    - OMSetRenderTargets()
///    - PSSetShaderResources()
///    - RSSetViewports()
///    - VSSetShader()
///    - PSSetShader()
///    - PSSetSamplers()
///    - VSSetConstantBuffers()
///    - PSSetConstantBuffers()
///    - RSSetState()
///    - OMSetDepthStencilState()
///    - OMSetBlendState()
///    - IASetInputLayout()
///    - IASetPrimitiveTopology()
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_DX11_ResolveFromMotionVectors(
	const NvTxaaResolveParametersDX11 *resolveParameters,
	const NvTxaaMotionDX11            *motion);

#endif // defined(__GFSDK_DX11__)

/*===========================================================================
TXAA OPENGL API FUNCTIONS
===========================================================================*/
#if defined(__GFSDK_GL__)

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Context.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaContextGL NvTxaaContextGL;

/////////////////////////////////////////////////////////////////////////////
/// @brief Programmable sample positions.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaSamplePositionsGL
{
    GLfloat xy[32];
} NvTxaaSamplePositionsGL;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Debug Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaDebugParametersGL
{
	/// <summary>
	/// Debug render target, valid if it:
	///   - Has the same dimensions (width and height) as msaaSource.
	///   - Is a view of a 2D texture.
	///   - Was created by the device that owns deviceContext.
	/// </summary>
	GLuint             target;
	
	/// <summary>
	/// Specifies what should be rendered into debugTarget
	/// </summary>
	NvTxaaDebugChannel channel;
} NvTxaaDebugParametersGL;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Resolve Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaResolveParametersGL
{	
	/// <summary>
	/// The TXAA context
	/// </summary>
	NvTxaaContextGL          *txaaContext;
	
	/// <summary>
	/// Target of the resolve; valid if it:
	///   - Is a 2D texture.
	/// </summary>
	GLuint                    resolveTarget;

	/// <summary>
	/// Source of the resolve; valid if it:
	///   - Has the same dimensions (width and height) as resolveTarget.
	///   - Is a Non-MSAA, 1X, 2X, 4X, or 8X MSAA 2D texture.
	/// </summary>
	GLuint                    msaaSource;

	/// <summary>
	/// Feedback from the prior frame (prior frame's resolveTarget); valid if it:
	///   - Has the same dimensions (width and height) as resolveTarget.
	///   - Is a 2D texture.
	/// </summary>
	GLuint                    feedbackSource;
	
	/// <summary>
	/// Texture used for per-pixel control of TXAA application; may be 0, valid if it:
	///   - Is a 2D texture.
	/// </summary>
	GLuint                    controlSource;
	
	/// <summary>
	/// The alpha resolve mode.
	/// </summary>
	NvTxaaAlphaResolveMode    alphaResolveMode;

	/// <summary>
	/// The compression range to use for resolve; null to disable compression.
	/// </summary>
	NvTxaaCompressionRange   *compressionRange;

	/// <summary>
	/// The feedback parameters to use for resolve; null for default feedback parameters.
	/// </summary>
	NvTxaaFeedbackParameters *feedback;

	/// <summary>
	/// The debug parameters to use for resolve; may be null.
	/// </summary>
	NvTxaaDebugParametersGL  *debug;
} NvTxaaResolveParametersGL;

/////////////////////////////////////////////////////////////////////////////
/// @brief TXAA Motion Parameters.
/////////////////////////////////////////////////////////////////////////////
typedef struct NvTxaaMotionGL
{		
	/// <summary>
	/// Velocity field; valid if it:
	/// - Has the same dimensions (width and height) as resolveTarget.
	/// - Is a 2D texture.
	/// </summary>
	GLuint                  motionVectors;
		
	/// <summary>
	/// The motion parameters; null to use default motion parameters.
	/// </summary>
	NvTxaaMotionParameters *parameters;
} NvTxaaMotionGL;

/////////////////////////////////////////////////////////////////////////////
/// @brief Initializes the TXAA library.
///
/// Note this might be a high latency operation as it does a GPU->CPU read.
///    - This is Metro safe.
///    - Might want to spawn a thread to do this async from normal load.
///
/// @param[in] txaaContext Pointer to an uninitialized NvTxaaContextGL
///            structure to be initialized
///
/// @param[in] version Pointer to an NvTxaaVersion structure; the application
///            should pass the address of the NvTxaaCurrentVersion constant
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if txaaContext is null;
///          NV_TXAA_STATUS_SDK_VERSION_MISMATCH if the application was compiled
///          against an incompatible version of the library;
///          NV_TXAA_STATUS_CONTEXT_ALREADY_INITIALIZED if txaaContext has
///          already been initialized;
///          NV_TXAA_STATUS_VERSION_MISMATCH if the application was compiled
///          against an incompatible version of the library;
///          NV_TXAA_STATUS_OK if initialization succeeded
///
/// @sideeffect modifies the following GL state:
///    - glUseProgram() 
///    - glBindFramebuffer(GL_FRAMEBUFFER, ...)
///    - glDrawBuffers()
///    - glActiveTexture(GL_TEXTURE0 through GL_TEXTURE2, ...)
///    - glBindTexture()
///     -glBindSampler()
///    - glViewport()
///    - glBindBuffer(GL_UNIFORM_BUFFER, ...)
///    - glPolygonMode()
///    - glBindBuffer(GL_UNIFORM_BUFFER, ...)
///    - glBindFramebuffer()
///    - glReadBuffer()
///    - glBindBuffer(GL_PIXEL_PACK_BUFFER)
///    - Assume the following state is changed,
///      (note these may change in the future)
///        - glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
///        - glDisable(GL_DEPTH_CLAMP)
///        - glDisable(GL_DEPTH_TEST) 
///        - glDisable(GL_CULL_FACE) 
///        - glDisable(GL_STENCIL_TEST)
///        - glDisable(GL_SCISSOR_TEST)
///        - glDisable(GL_POLYGON_OFFSET_POINT)
///        - glDisable(GL_POLYGON_OFFSET_LINE)
///        - glDisable(GL_POLYGON_OFFSET_FILL)
///        - glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE)
///        - glDisable(GL_LINE_SMOOTH)
///        - glDisable(GL_MULTISAMPLE)
///        - glDisable(GL_SAMPLE_MASK)
///        - glDisablei(GL_BLEND, 0)
///        - glDisablei(GL_BLEND, 1)
///        - glColorMaski(0, 1,1,1,1)
///        - glColorMaski(1, 1,1,1,1)
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_GL_InitializeContext(
	NvTxaaContextGL     *txaaContext,
	const NvTxaaVersion *version = &NvTxaaCurrentVersion);

/////////////////////////////////////////////////////////////////////////////
/// @brief Releases the specified TXAA context.
///
/// @param[in] txaaContext Pointer to an initialized NvTxaaContextGL
///            structure to be released
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if txaaContext is null;
///          NV_TXAA_STATUS_CONTEXT_NOT_INITIALIZED if txaaContext has not
///          been initialized with a call to GFSDK_TXAA_GL_InitializeContext;
///          NV_TXAA_STATUS_OK if the context has been fully released
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_GL_ReleaseContext(
	NvTxaaContextGL *txaaContext);

/////////////////////////////////////////////////////////////////////////////
/// @brief Resolves the supplied mutisample surface using a supplied motion
/// vector field. If non-MSAA surface is provided, this will triger a
/// non-MSAA mode TXAA resolve.
/// 
/// Use in-place of standard MSAA resolve.
///
/// The supplied control texture can be used to control the amount of
/// antialiasing [0-1]. The filter is narrowed and temporal feedback is
/// limited in areas with lower control values. If not supplied, all pixels
/// use the widest filter and maximum temporal feedback.
///
/// @param[in] resolveParameters Resolve parameters
///
/// @param[in] motion Motion data
///
/// @returns NV_TXAA_STATUS_INVALID_ARGUMENT if resolveParameters or motionVectors is null;
///          NV_TXAA_STATUS_CONTEXT_NOT_INITIALIZED if resolveParameters->txaaContext has not
///          been initialized with a call to GFSDK_TXAA_GL_InitializeContext;
///          NV_TXAA_STATUS_INVALID_RESOLVETARGET if resolveParameters->resolveTarget is invalid;
///          NV_TXAA_STATUS_INVALID_MSAASOURCE if resolveParameters->msaaSource is invalid;
///          NV_TXAA_STATUS_INVALID_FEEDBACKSOURCE if resolveParameters->feedback is invalid;
///          NV_TXAA_STATUS_INVALID_CONTROLSOURCE if resolveParameters->controlSource is invalid;
///          NV_TXAA_STATUS_INVALID_COMPRESSIONRANGE if resolveParameters->compressionRange is invalid;
///          NV_TXAA_STATUS_INVALID_MOTIONVECTORS if motionVectors is invalid;
///          NV_TXAA_STATUS_OK if the resolve succeeded
///
/// @sideeffect modifies the following GL state:
///    - glUseProgram() 
///    - glBindFramebuffer(GL_FRAMEBUFFER, ...)
///    - glDrawBuffers()
///    - glActiveTexture(GL_TEXTURE0 through GL_TEXTURE2, ...)
///    - glBindTexture()
///     -glBindSampler()
///    - glViewport()
///    - glBindBuffer(GL_UNIFORM_BUFFER, ...)
///    - glPolygonMode()
///    - Assume the following state is changed,
///      (note these may change in the future)
///        - glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
///        - glDisable(GL_DEPTH_CLAMP)
///        - glDisable(GL_DEPTH_TEST) 
///        - glDisable(GL_CULL_FACE) 
///        - glDisable(GL_STENCIL_TEST)
///        - glDisable(GL_SCISSOR_TEST)
///        - glDisable(GL_POLYGON_OFFSET_POINT)
///        - glDisable(GL_POLYGON_OFFSET_LINE)
///        - glDisable(GL_POLYGON_OFFSET_FILL)
///        - glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE)
///        - glDisable(GL_LINE_SMOOTH)
///        - glDisable(GL_MULTISAMPLE)
///        - glDisable(GL_SAMPLE_MASK)
///        - glDisablei(GL_BLEND, 0)
///        - glDisablei(GL_BLEND, 1)
///        - glColorMaski(0, 1,1,1,1)
///        - glColorMaski(1, 1,1,1,1)
/////////////////////////////////////////////////////////////////////////////
TXAA_API NvTxaaStatus GFSDK_TXAA_GL_ResolveFromMotionVectors(
	const NvTxaaResolveParametersGL *resolveParameters,
	const NvTxaaMotionGL            *motion);

#endif // defined(__GFSDK_GL__)

#if defined(__cplusplus)
} // extern "C"
#endif

/*===========================================================================
TXAA CONTEXT DEFINITIONS
===========================================================================*/
#if !defined(__TXAA_CPP__)
#	if defined(__GFSDK_DX11__)
	struct NvTxaaContextDX11 { __GFSDK_ALIGN__(64) gfsdk_U8 pad[8192]; };
#	endif
#	if defined(__GFSDK_GL__)
	struct NvTxaaContextGL { __GFSDK_ALIGN__(64) gfsdk_U8 pad[8292]; };
#	endif
#endif

#endif // !defined(__GFSDK_TXAA_H__)
