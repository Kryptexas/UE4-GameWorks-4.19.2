// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// This code is modified from that in the Mesa3D Graphics library available at
// http://mesa3d.org/
// The license for the original code follows:

/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** mesa/main/compiler.h */

/**
 * \file compiler.h
 * Compiler-related stuff.
 */


#ifndef COMPILER_H
#define COMPILER_H


//@todo-rco: Remove STL!
#include <ctype.h>
#include <stdarg.h>


/**
 * Get standard integer types
 */
#include <stdint.h>

/**
 * Functions that have different names in Visual Studio
 */
#ifndef _MSC_VER
#define stricmp strcasecmp
#endif


#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf _snprintf
#endif

/** Minimum of two values: */
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )

/** Maximum of two values: */
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

#ifndef Elements
#define Elements(x) (sizeof(x)/sizeof(*(x)))
#endif

#endif /* COMPILER_H */
