/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

/* 
*   █████  █████ ██████ ████  ████   ███████   ████  ██████ ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██████ ████    ██   ████  █████  ██ ██ ██ ██████   ██   ███████
*   ██  ██ ██      ██   ██    ██  ██ ██    ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   █████ ██  ██ ██    ██ ██  ██   ██   ██   ██   DEBUGGER
*                                                           ██   ██
*  ████████████████████████████████████████████████████████ ██ █ ██ ████████████ 
*
*
*       
*  HOW TO USE:
*           
*  1)  Call, 'GFSDK_Aftermath_DXxx_Initialize', to initialize the library.            
*      This must be done before any other library calls are made, and the method
*      must return 'GFSDK_Aftermath_Result_Success' for initialization to
*      be complete.
*
*
*  2)  For each commandlist/device context you expect to use with Aftermath,
*      initialize them using the 'GFSDK_Aftermath_DXxx_CreateContextHandle',
*      function.
*
*
*  3)  Call 'GFSDK_Aftermath_SetEventMarker', to inject an event 
*      marker directly into the command stream at that point.
*
*
*  4)  Once TDR/hang occurs, call the 'GFSDK_Aftermath_GetData' API 
*      to fetch the event marker last processed by the GPU for each context.
*      This API also supports fetching the current execution state for each
*      the GPU.
*
*
*  5)  Before the app shuts down, each Aftermath context handle must be cleaned
*      up, this is done with the 'GFSDK_Aftermath_ReleaseContextHandle' call.
*
*  OPTIONAL: 
*
*  o)  To query the fault reason after TDR, use the 'GFSDK_Aftermath_GetDeviceStatus' 
*      call.  See 'GFSDK_Aftermath_Device_Status', for the full list of possible status.
*
*
*  o)  In the event of a GPU page fault, use the 'GFSDK_Aftermath_GetPageFaultInformation' 
*      method to return more information about what might of gone wrong.
*      A GPU VA is returned, along with the resource descriptor of the resource that VA
*      lands in.  NOTE: It's not 100% certain that this is the resource which caused the
*      fault, only that the faulting VA lands within this resource in memory.
*
*
*  PERFORMANCE TIPS:
*
*  o) For maximum CPU performance, use 'GFSDK_Aftermath_SetEventMarker' with dataSize=0.
*     this instructs Aftermath not to allocate and copy off memory internally, relying on
*     the application to manage marker pointers itself.
*
*
*      Contact: Alex Dunn <adunn@nvidia.com>
*/

#ifndef GFSDK_Aftermath_H
#define GFSDK_Aftermath_H

#include "defines.h"

enum GFSDK_Aftermath_Version { GFSDK_Aftermath_Version_API = 0x0000013 }; // Version 1.3

enum GFSDK_Aftermath_Result
{
    GFSDK_Aftermath_Result_Success = 0x1,
        
    GFSDK_Aftermath_Result_Fail = 0xBAD00000,

    // The callee tries to use a library version
    //  which does not match the built binary.
    GFSDK_Aftermath_Result_FAIL_VersionMismatch = GFSDK_Aftermath_Result_Fail | 1,

    // The library hasn't been initialized, see;
    //  'GFSDK_Aftermath_Initialize'.
    GFSDK_Aftermath_Result_FAIL_NotInitialized = GFSDK_Aftermath_Result_Fail | 2,

    // The callee tries to use the library with 
    //  a non-supported GPU.  Currently, only 
    //  NVIDIA GPUs are supported.
    GFSDK_Aftermath_Result_FAIL_InvalidAdapter = GFSDK_Aftermath_Result_Fail | 3,

    // The callee passed an invalid parameter to the 
    //  library, likely a null pointer or bad handle.
    GFSDK_Aftermath_Result_FAIL_InvalidParameter = GFSDK_Aftermath_Result_Fail | 4,

    // Something weird happened that caused the 
    //  library to fail for some reason.
    GFSDK_Aftermath_Result_FAIL_Unknown = GFSDK_Aftermath_Result_Fail | 5,

    // Got a fail error code from the graphics API.
    GFSDK_Aftermath_Result_FAIL_ApiError = GFSDK_Aftermath_Result_Fail | 6,

    // Make sure that the NvAPI DLL is up to date.
    GFSDK_Aftermath_Result_FAIL_NvApiIncompatible = GFSDK_Aftermath_Result_Fail | 7,

    // It would appear as though a call has been
    //  made to fetch the Aftermath data for a 
    //  context that hasn't been used with 
    //  the EventMarker API yet.
    GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList = GFSDK_Aftermath_Result_Fail | 8,

    // Looks like the library has already been initialized.
     GFSDK_Aftermath_Result_FAIL_AlreadyInitialized = GFSDK_Aftermath_Result_Fail | 9,

    // Debug layer not compatible with Aftermath.
    GFSDK_Aftermath_Result_FAIL_D3DDebugLayerNotCompatible = GFSDK_Aftermath_Result_Fail | 10,

    // Aftermath failed to initialize in the driver.
    GFSDK_Aftermath_Result_FAIL_DriverInitFailed = GFSDK_Aftermath_Result_Fail | 11,

    // Aftermath v1.25 requires driver version 387.xx and beyond
    GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported = GFSDK_Aftermath_Result_Fail | 12,

    // The system ran out of memory for allocations
    GFSDK_Aftermath_Result_FAIL_OutOfMemory = GFSDK_Aftermath_Result_Fail | 13,

    // No need to get data on bundles, as markers 
    //  execute on the command list.
    GFSDK_Aftermath_Result_FAIL_GetDataOnBundle = GFSDK_Aftermath_Result_Fail | 14,

    // No need to get data on deferred contexts, as markers 
    //  execute on the immediate context.
    GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext = GFSDK_Aftermath_Result_Fail | 15,

    // This feature hasn't been enabled at initialization - see GFSDK_Aftermath_FeatureFlags.
    GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled = GFSDK_Aftermath_Result_Fail | 16,
};

#define GFSDK_Aftermath_SUCCEED(value) (((value) & 0xFFF00000) != GFSDK_Aftermath_Result_Fail)


// Here is defined a set of features that can be enabled/disable when using Aftermath
enum GFSDK_Aftermath_FeatureFlags
{
    // The minimal flag only allows use of the GetDeviceStatus entry point.
    GFSDK_Aftermath_FeatureFlags_Minimum = 0,

    // With this flag set, the SetEventMarker and GetData entry points are available.
    GFSDK_Aftermath_FeatureFlags_EnableMarkers = 1,

    // With this flag set, resources are tracked, and information about
    //  possible page fault candidates can be accessed using GetPageFaultInformation.
    GFSDK_Aftermath_FeatureFlags_EnableResourceTracking = 2,

    // Use all Aftermath features
    GFSDK_Aftermath_FeatureFlags_Maximum = GFSDK_Aftermath_FeatureFlags_Minimum | GFSDK_Aftermath_FeatureFlags_EnableMarkers | GFSDK_Aftermath_FeatureFlags_EnableResourceTracking,
};


enum GFSDK_Aftermath_Context_Status
{
    // GPU:
    // The GPU has not started processing this command list yet.
    GFSDK_Aftermath_Context_Status_NotStarted = 0,

    // This command list has begun execution on the GPU.
    GFSDK_Aftermath_Context_Status_Executing,

    // This command list has finished execution on the GPU.
    GFSDK_Aftermath_Context_Status_Finished,

    // This context has an invalid state, which could be
    //  caused by an error.  
    //
    //  NOTE: See, 'GFSDK_Aftermath_ContextData::getErrorCode()'
    //  for more information.
    GFSDK_Aftermath_Context_Status_Invalid,
};


enum GFSDK_Aftermath_Device_Status
{
    // The GPU is still active, and hasn't gone down.
    GFSDK_Aftermath_Device_Status_Active = 0,

    // A long running shader/operation has caused a 
    //  GPU timeout. Reconfiguring the timeout length
    //  might help tease out the problem.
    GFSDK_Aftermath_Device_Status_Timeout,

    // Run out of memory to complete operations.
    GFSDK_Aftermath_Device_Status_OutOfMemory,

    // An invalid VA access has caused a fault.
    GFSDK_Aftermath_Device_Status_PageFault,

    // Unknown problem - likely using an older driver
    //  incompatible with this Aftermath feature.
    GFSDK_Aftermath_Device_Status_Unknown,
};


// Used with Aftermath entry points to reference an API object.
AFTERMATH_DECLARE_HANDLE(GFSDK_Aftermath_ContextHandle);


// Used with, 'GFSDK_Aftermath_GetData'.  Filled with information,
//  about each requested context.
struct GFSDK_Aftermath_ContextData
{
    void* markerData;
    unsigned int markerSize;
    GFSDK_Aftermath_Context_Status status;

    // Call this when 'status' is 'GFSDK_Aftermath_Context_Status_Invalid;
    //  to determine what the error failure reason is.
    GFSDK_Aftermath_Result getErrorCode()
    {
        if (status == GFSDK_Aftermath_Context_Status_Invalid)
        {
            return (GFSDK_Aftermath_Result)(uintptr_t)markerData;
        }

        return GFSDK_Aftermath_Result_Success;
    }
};

// Minimal description of a graphics resource.
struct GFSDK_Aftermath_ResourceDescriptor
{
    const void* pAppRes; // Not currently available in the driver, expect support in a future Aftermath

    UINT64 size;

    UINT width;
    UINT height;
    UINT depth;

    UINT16 mipLevels;

    DXGI_FORMAT format;

    bool bIsBufferHeap : 1;
    bool bIsStaticTextureHeap : 1;
    bool bIsRtvDsvTextureHeap : 1;
    bool bPlacedResource : 1;

    bool bWasDestroyed : 1;
};

// Used with GFSDK_Aftermath_GetPageFaultInformation
struct GFSDK_Aftermath_PageFaultInformation
{
    UINT64 faultingGpuVA;
    GFSDK_Aftermath_ResourceDescriptor resourceDesc;
    bool bhasPageFaultOccured : 1;
};


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_DX11_Initialize
// GFSDK_Aftermath_DX12_Initialize
// ---------------------------------
//
// [pDx11Device]; DX11-Only
//      the current dx11 device pointer.
//
// [pDx12Device]; DX12-Only
//      the current dx12 device pointer.
//
// flags;
//      set of features to enable when initializing Aftermath
//
// version;
//      use the version supplied in this header - library will match 
//      that with what it believes to be the version internally.
//
//// DESCRIPTION;
//      Library must be initialized before any other call is made.
//      This should be done after device creation.
// 
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX11_Initialize(GFSDK_Aftermath_Version version, GFSDK_Aftermath_FeatureFlags flags, ID3D11Device* const pDx11Device);
#endif
#ifdef __d3d12_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version version, GFSDK_Aftermath_FeatureFlags flags, ID3D12Device* const pDx12Device);
#endif


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_DX11_CreateContextHandle
// GFSDK_Aftermath_DX12_CreateContextHandle
// ---------------------------------
//
// (pDx11DeviceContext); DX11-Only
//      Device context to use with Aftermath.
//
// (pDx12CommandList); DX12-Only
//      Command list to use with Aftermath
//
// pOutContextHandle;
//      The context handle for the specified context/command list
//      to be used with future Aftermath calls.
//
//// DESCRIPTION;
//      Before Aftermath event markers can be inserted, 
//      a context handle reference must first be fetched.
// 
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX11_CreateContextHandle(ID3D11DeviceContext* const pDx11DeviceContext, GFSDK_Aftermath_ContextHandle* pOutContextHandle);
#endif             
#ifdef __d3d12_h__ 
GFSDK_Aftermath_API GFSDK_Aftermath_DX12_CreateContextHandle(ID3D12GraphicsCommandList* const pDx12CommandList, GFSDK_Aftermath_ContextHandle* pOutContextHandle);
#endif


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_SetEventMarker
// -------------------------------------
// 
// contextHandle; 
//      Command list currently being populated. 
// 
// markerData;
//      Pointer to data used for event marker.
//      NOTE: An internal copy will be made of this data, no 
//      need to keep it around after this call - stack alloc is safe.
//
// markerSize;
//      Size of event in bytes.
//      NOTE:   Passing a 0 for this parameter is valid, and will
//              only copy off the ptr supplied by markerData, rather
//              than internally making a copy.
//              NOTE:   This is a requirement for deferred contexts on D3D11.
//
// DESCRIPTION;
//      Drops a event into the command stream with a payload that can be 
//      linked back to the data given here, 'markerData'.  It's 
//      safe to call from multiple threads simultaneously, normal D3D 
//      API threading restrictions apply.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_ReleaseContextHandle(const GFSDK_Aftermath_ContextHandle contextHandle);


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_SetEventMarker
// -------------------------------------
// 
// contextHandle; 
//      Command list currently being populated. 
// 
// markerData;
//      Pointer to data used for event marker.
//      NOTE: An internal copy will be made of this data, no 
//      need to keep it around after this call - stack alloc is safe.
//
// markerSize;
//      Size of event in bytes.
//      NOTE:   Passing a 0 for this parameter is valid, and will
//              only copy off the ptr supplied by markerData, rather
//              than internally making a copy.
//
// DESCRIPTION;
//      Drops a event into the command stream with a payload that can be 
//      linked back to the data given here, 'markerData'.  It's 
//      safe to call from multiple threads simultaneously, normal D3D 
//      API threading restrictions apply.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_SetEventMarker(const GFSDK_Aftermath_ContextHandle contextHandle, const void* markerData, const unsigned int markerSize);


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetData
// ------------------------------
// 
// numContexts;
//      Number of contexts to fetch information for.
//      NOTE:   Passing a 0 for this parameter will only 
//              return the GPU status in pStatusOut.
//
// pContextHandles;
//      Array of contexts containing Aftermath event markers. 
// 
// pOutContextData;
//      OUTPUT: context data for each context requested. Contains event
//              last reached on the GPU, and status of context if
//              applicable (DX12-Only).
//      NOTE: must allocate enough space for 'numContexts' worth of structures.
//            stack allocation is fine.
//
// pStatusOut;
//      OUTPUT: the current status of the GPU.
//
// DESCRIPTION;
//      Once a TDR/crash/hang has occurred (or whenever you like), call 
//      this API to retrieve the event last processed by the GPU on the 
//      given context.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GetData(const unsigned int numContexts, const GFSDK_Aftermath_ContextHandle* pContextHandles, GFSDK_Aftermath_ContextData* pOutContextData);


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetDeviceStatus
// ---------------------------------
//
// pOutStatus;
//      OUTPUT: Device status.
//
//// DESCRIPTION;
//      Return the status of a D3D device.  See GFSDK_Aftermath_Device_Status.
// 
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GetDeviceStatus(GFSDK_Aftermath_Device_Status* pOutStatus);


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetPageFaultInformation
// ---------------------------------
//
// pOutPageFaultInformation;
//      OUTPUT: Information about a page fault which may have occurred.
//
//// DESCRIPTION;
//      Return any information available about a recent page fault which 
//      may have occurred, causing a device removed scenario.  
//      See GFSDK_Aftermath_PageFaultInformation.
// 
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GetPageFaultInformation(GFSDK_Aftermath_PageFaultInformation* pOutPageFaultInformation);


/////////////////////////////////////////////////////////////////////////
//
// NOTE: Function table provided - if dynamic loading is preferred.
//
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX11_Initialize)(GFSDK_Aftermath_Version version, GFSDK_Aftermath_FeatureFlags flags, ID3D11Device* const pDx11Device);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX11_CreateContextHandle)(ID3D11DeviceContext* const pDx11DeviceContext, GFSDK_Aftermath_ContextHandle* pOutContextHandle);
#endif

#ifdef __d3d12_h__
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX12_Initialize)(GFSDK_Aftermath_Version version, GFSDK_Aftermath_FeatureFlags flags, ID3D12Device* const pDx12Device);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX12_CreateContextHandle)(ID3D12CommandList* const pDx12CommandList, GFSDK_Aftermath_ContextHandle* pOutContextHandle);
#endif

GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_ReleaseContextHandle)(const GFSDK_Aftermath_ContextHandle contextHandle);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_SetEventMarker)(const GFSDK_Aftermath_ContextHandle contextHandle, const void* markerData, const unsigned int markerSize);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_GetData)(const unsigned int numContexts, const GFSDK_Aftermath_ContextHandle* ppContextHandles, GFSDK_Aftermath_ContextData* pOutContextData);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_GetDeviceStatus)(GFSDK_Aftermath_Device_Status* pOutStatus);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_GetPageFaultInformation)(GFSDK_Aftermath_PageFaultInformation* pOutPageFaultInformation);

#endif // GFSDK_Aftermath_H