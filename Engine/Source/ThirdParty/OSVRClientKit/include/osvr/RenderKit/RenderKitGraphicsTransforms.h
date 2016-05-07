/** @file
@brief Header file describing the OSVR graphics transformations interface

@date 2015

@author
Russ Taylor <russ@sensics.com>
<http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// Internal Includes
#include <osvr/RenderKit/Export.h>

// Library/third-party includes
#include <osvr/Util/ClientReportTypesC.h>

// Standard includes
#include <memory>

namespace osvr {
namespace renderkit {

    //=========================================================================
    /// Description needed to construct an off-axes projection matrix
    typedef struct {
        double left;
        double right;
        double top;
        double bottom;
        double nearClip; //< Cannot name "near" because Visual Studio keyword
        double farClip;
    } OSVR_ProjectionMatrix;

    //=========================================================================
    /// Viewport description with lower-left corner of the screen as (0,0)
    struct OSVR_ViewportDescription {
        double left;   //< Left side of the viewport in pixels
        double lower;  //< First pixel in the viewport at the bottom.
        double width;  //< Last pixel in the viewport at the top
        double height; //< Last pixel on the right of the viewport in pixels

        bool operator==(const OSVR_ViewportDescription& v) const {
            return (v.left == left) && (v.lower == lower) &&
                   (v.width == width) && (v.height == height);
        }

        bool operator!=(const OSVR_ViewportDescription& v) const {
            return (v.left != left) || (v.lower != lower) ||
                   (v.width != width) || (v.height != height);
        }
    };

    //=========================================================================
    // Routines to turn the OSVR_PoseState into ModelView matrices for OpenGL
    // and
    // Direct3D.  Done in such a way that we don't require the inclusion of the
    // native API header files (since most apps will not include all of the
    // libraries).

    /// @brief Produce OpenGL ModelView transform from OSVR_PoseState
    bool OSVR_RENDERMANAGER_EXPORT OSVR_PoseState_to_OpenGL(
        double* OpenGL_out, const OSVR_PoseState& state_in);
    /// @brief Produce D3D ModelView transform from OSVR_PoseState
    bool OSVR_RENDERMANAGER_EXPORT
    OSVR_PoseState_to_D3D(float D3D_out[16], const OSVR_PoseState& state_in);

    //=========================================================================
    // Routines to turn the 4x4 projection matrices returned as part of the
    // RenderCallback class into Projection matrices for OpenGL and
    // Direct3D.  Done in such a way that we don't require the inclusion of the
    // native API header files (since most apps will not include all of the
    // libraries).

    /// @brief Produce OpenGL Projection matrix from 4x4 projection matrix
    bool OSVR_RENDERMANAGER_EXPORT OSVR_Projection_to_OpenGL(
        double* OpenGL_out, OSVR_ProjectionMatrix projection_in);
    /// @brief Produce Direct3D Projection matrix from 4x4 projection matrix
    bool OSVR_RENDERMANAGER_EXPORT OSVR_Projection_to_D3D(
        float D3D_out[16], OSVR_ProjectionMatrix projection_in);

    //=========================================================================
    // Routines to turn the OSVR viewpoint descriptor into appropriate values
    // for OpenGL and Direct3D (which have different Normalized Device
    // Coordinates).

    // See
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb206341%28v=vs.85%29.aspx
    /// @todo

} // namespace renderkit
} // namespace osvr
