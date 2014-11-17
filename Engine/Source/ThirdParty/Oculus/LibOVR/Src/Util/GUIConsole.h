/************************************************************************************

Filename    :   GUIConsole.h
Content     :   A stdout console window that runs alongside Windows GUI applications
Created     :   Feb 4, 2013
Authors     :   Brant Lewis

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.1 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.1 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#ifndef GUICONSOLE_H
#define GUICONSOLE_H

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "../../Include/OVR.h"

#ifdef OVR_INTERNAL_USE

class GUIConsole
{
public:
  // constructors
  GUIConsole();
  ~GUIConsole();

  // member variables
  HANDLE hStdIn, hStdOut, hStdError;
};
#endif // #ifdef OVR_INTERNAL_USE

#endif