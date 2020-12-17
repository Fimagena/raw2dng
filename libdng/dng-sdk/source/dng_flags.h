/*****************************************************************************/
// Copyright 2006-2019 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

/** \file
 * Conditional compilation flags for DNG SDK.
 *
 * All conditional compilation macros for the DNG SDK begin with a lowercase 'q'.
 */

/*****************************************************************************/

#ifndef __dng_flags__
#define __dng_flags__

/*****************************************************************************/

/// \def qMacOS 
/// 1 if compiling for Mac OS X.

/// \def qWinOS 
/// 1 if compiling for Windows.

// Make sure a platform is defined

#if !(defined(qMacOS) || defined(qWinOS) || defined(qAndroid) || defined(qiPhone) || defined(qLinux))
#include "RawEnvironment.h"
#endif

// This requires a force include or compiler define.  These are the unique platforms.

#if !(defined(qMacOS) || defined(qWinOS) || defined(qAndroid) || defined(qiPhone) || defined(qLinux))
#error Unable to figure out platform
#endif

/*****************************************************************************/

// Platforms.
// Zeros out any undefined platforms, so that #if can be used in place of #ifdef.

#ifndef qMacOS
#define qMacOS 0
#endif

#ifndef qiPhone
#define qiPhone 0
#endif

#ifndef qiPhoneSimulator
#define qiPhoneSimulator 0
#endif

#ifndef qAndroid
#define qAndroid 0
#endif

#ifndef qWinOS
#define qWinOS 0
#endif

#ifndef qWinRT
#define qWinRT 0
#endif

#ifndef qLinux
#define qLinux 0
#endif

#ifndef qWeb
#define qWeb 0
#endif

/*****************************************************************************/

#if qiPhoneSimulator
#if !qiPhone
#error "qiPhoneSimulator set and not qiPhone"
#endif
#endif

#if qWinRT
#if !qWinOS
#error "qWinRT set but not qWinOS"
#endif
#endif

/*****************************************************************************/

// arm and neon support

// arm detect (apple vs. win)
#if defined(__arm__) || defined(__arm64__) || defined(_M_ARM)
#define qARM 1
#endif

// arm_neon detect
#if defined(__ARM_NEON__) || defined(_M_ARM)
#define qARMNeon 1
#endif

#ifndef qARM 
#define qARM 0
#endif

#ifndef qARMNeon
#define qARMNeon 0
#endif

/*****************************************************************************/

// Establish WIN32 and WIN64 definitions.

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 1
#endif

#if defined(_WIN64) && !defined(WIN64)
#define WIN64 1
#endif

/*****************************************************************************/

/// \def qDNGDebug 
/// 1 if debug code is compiled in, 0 otherwise. Enables assertions and other debug
/// checks in exchange for slower processing.

// Figure out if debug build or not.

#ifndef qDNGDebug

#if defined(Debug)
#define qDNGDebug Debug

#elif defined(_DEBUG)
#define qDNGDebug _DEBUG

#else
#define qDNGDebug 0

#endif
#endif

/*****************************************************************************/
// Support Intel Thread Building Blocks (TBB)?
// 
// This flag needs to be configured via the project, because there are sources
// outside the cr_sdk (such as the CTJPEG and ACE libs) that need to use the
// same flag to determine whether to use TBB or not.
// 
// By default, configure to 0 (disabled).

#ifndef qCRSupportTBB
#define qCRSupportTBB 0
#endif

#if qCRSupportTBB
#ifndef TBB_DEPRECATED
#define TBB_DEPRECATED 0
#endif
#endif

// This is not really a switch, but rather a shorthand for determining whether
// or not we're building a particular translation unit (source file) using the
// Intel Compiler.

#ifndef qDNGIntelCompiler
#if defined(__INTEL_COMPILER)
#define qDNGIntelCompiler (__INTEL_COMPILER >= 1700)
#else
#define qDNGIntelCompiler 0
#endif
#endif

/*****************************************************************************/

// Figure out byte order.

/// \def qDNGBigEndian 
/// 1 if this target platform is big endian (e.g. PowerPC Macintosh), else 0.
///
/// \def qDNGLittleEndian 
/// 1 if this target platform is little endian (e.g. x86 processors), else 0.

#ifndef qDNGBigEndian

#if defined(qDNGLittleEndian)
#define qDNGBigEndian !qDNGLittleEndian

#elif defined(__POWERPC__)
#define qDNGBigEndian 1

#elif defined(__INTEL__)
#define qDNGBigEndian 0

#elif defined(_M_IX86)
#define qDNGBigEndian 0

#elif defined(_M_X64) || defined(__amd64__)
#define qDNGBigEndian 0

#elif defined(__LITTLE_ENDIAN__)
#define qDNGBigEndian 0

#elif defined(__BIG_ENDIAN__)
#define qDNGBigEndian 1

#elif defined(_ARM_)
#define qDNGBigEndian 0

#else

#ifndef qXCodeRez
#error Unable to figure out byte order.
#endif

#endif
#endif

#ifndef qXCodeRez

#ifndef qDNGLittleEndian
#define qDNGLittleEndian !qDNGBigEndian
#endif

#endif

/*****************************************************************************/

/// \def qDNG64Bit 
/// 1 if this target platform uses 64-bit addresses, 0 otherwise.

#ifndef qDNG64Bit

#if qMacOS

#ifdef __LP64__
#if    __LP64__
#define qDNG64Bit 1
#endif
#endif

#elif qWinOS

#ifdef WIN64
#if    WIN64
#define qDNG64Bit 1
#endif
#endif

#elif qLinux

#ifdef __LP64__
#if    __LP64__
#define qDNG64Bit 1
#endif
#endif

#endif

#ifndef qDNG64Bit
#define qDNG64Bit 0
#endif

#endif

/*****************************************************************************/

/// \def qDNGThreadSafe 
/// 1 if target platform has thread support and threadsafe libraries, 0 otherwise.

#ifndef qDNGThreadSafe
#define qDNGThreadSafe (qMacOS || qWinOS)
#endif

/*****************************************************************************/

/// \def qDNGValidateTarget 
/// 1 if dng_validate command line tool is being built, 0 otherwise.

#ifndef qDNGValidateTarget
#define qDNGValidateTarget 0
#endif

/*****************************************************************************/

/// \def qDNGValidate 
/// 1 if DNG validation code is enabled, 0 otherwise.

#ifndef qDNGValidate
#define qDNGValidate qDNGValidateTarget
#endif

/*****************************************************************************/

/// \def qDNGPrintMessages 
/// 1 if dng_show_message should use fprintf to stderr. 0 if it should use a platform
/// specific interrupt mechanism.

#ifndef qDNGPrintMessages
#define qDNGPrintMessages qDNGValidate
#endif

/*****************************************************************************/

// Experimental features -- work in progress for Lightroom and Camera Raw
// major releases. Turn this off for Lightroom & Camera Raw dot releases.

#ifndef qDNGExperimental
#define qDNGExperimental 1
#endif

/*****************************************************************************/

/// \def qDNGXMPFiles 
/// 1 to use XMPFiles.

#ifndef qDNGXMPFiles
#define qDNGXMPFiles 1
#endif

/*****************************************************************************/

/// \def qDNGXMPDocOps 
/// 1 to use XMPDocOps.

#ifndef qDNGXMPDocOps
#define qDNGXMPDocOps (!qDNGValidateTarget)
#endif

/*****************************************************************************/

/// \def qDNGUseLibJPEG
/// 1 to use open-source libjpeg for lossy jpeg processing.

#ifndef qDNGUseLibJPEG
#define qDNGUseLibJPEG qDNGValidateTarget
#endif

/*****************************************************************************/

#ifndef qDNGAVXSupport
#define qDNGAVXSupport ((qMacOS || qWinOS) && qDNG64Bit && !qARM && 1)
#endif

#if qDNGAVXSupport && !(qDNG64Bit && !qARM)
#error AVX support is enabled when 64-bit support is not or ARM is
#endif

/*****************************************************************************/

#ifndef qDNGSupportVC5
#define qDNGSupportVC5 (1)
#endif

/*****************************************************************************/

/// \def qDNGUsingSanitizer
/// Set to 1 when using a Sanitizer tool.

#ifndef qDNGUsingSanitizer
#define qDNGUsingSanitizer (0)
#endif

/*****************************************************************************/

#ifndef DNG_ATTRIB_NO_SANITIZE
#if qDNGUsingSanitizer && defined(__clang__)
#define DNG_ATTRIB_NO_SANITIZE(type) __attribute__((no_sanitize(type)))
#else
#define DNG_ATTRIB_NO_SANITIZE(type)
#endif
#endif

/*****************************************************************************/

/// \def qDNGDepthSupport
/// 1 to add support for depth maps in DNG format.
/// Deprecated 2018-09-19.

#ifdef __cplusplus
#define qDNGDepthSupport #error
#endif

/*****************************************************************************/

/// \def qDNGPreserveBlackPoint
/// 1 to add support for non-zero black point in early raw pipeline.
/// Deprecated 2018-09-19.

#ifdef __cplusplus
#define qDNGPreserveBlackPoint #error
#endif

/*****************************************************************************/

#endif
	
/*****************************************************************************/
