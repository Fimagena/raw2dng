/*****************************************************************************/
// Copyright 2007-2019 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

#include "dng_hue_sat_map.h"

#include "dng_assertions.h"
#include "dng_auto_ptr.h"
#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_host.h"

/*****************************************************************************/

std::atomic<uint64> dng_hue_sat_map::sRuntimeFingerprintCounter (0);

/*****************************************************************************/

dng_hue_sat_map::dng_hue_sat_map ()

	:	fHueDivisions       (0)
	,	fSatDivisions       (0)
	,	fValDivisions       (0)
	,	fHueStep            (0)
	,	fValStep	        (0)
	,	fRuntimeFingerprint ()
	,	fDeltas             ()

	{
	
	}

/*****************************************************************************/

dng_hue_sat_map::dng_hue_sat_map (const dng_hue_sat_map &src)

	:	fHueDivisions       (0)
	,	fSatDivisions       (0)
	,	fValDivisions       (0)
	,	fHueStep            (0)
	,	fValStep	        (0)
	,	fRuntimeFingerprint ()
	,	fDeltas             ()

	{
	
	*this = src;
	
	}

/*****************************************************************************/

dng_hue_sat_map &dng_hue_sat_map::operator= (const dng_hue_sat_map &rhs)
	{ 

	if (this != &rhs)
		{

		if (!rhs.IsValid ())
			{

			SetInvalid ();

			}

		else
			{

			fHueDivisions = rhs.fHueDivisions;
			fSatDivisions = rhs.fSatDivisions;
			fValDivisions = rhs.fValDivisions;

			fHueStep = rhs.fHueStep;
			fValStep = rhs.fValStep;

			fRuntimeFingerprint = rhs.fRuntimeFingerprint;

			fDeltas = rhs.fDeltas;

			}

		}

	return *this;

	}

/*****************************************************************************/

dng_hue_sat_map::~dng_hue_sat_map ()
	{
	
	}

/*****************************************************************************/

void dng_hue_sat_map::SetDivisions (uint32 hueDivisions,
									uint32 satDivisions,
									uint32 valDivisions)
	{

	DNG_ASSERT (hueDivisions >= 1, "Must have at least 1 hue division.");
	DNG_ASSERT (satDivisions >= 2, "Must have at least 2 sat divisions.");
	
	if (valDivisions == 0)
		valDivisions = 1;

	if (hueDivisions == fHueDivisions &&
		satDivisions == fSatDivisions &&
		valDivisions == fValDivisions)
		{
		return;
		}

	fHueDivisions = hueDivisions;
	fSatDivisions = satDivisions;
	fValDivisions = valDivisions;
	
	fHueStep = satDivisions;
	fValStep = hueDivisions * fHueStep;

	dng_safe_uint32 size (DeltasCount ());

	size *= (uint32) sizeof (HSBModify);
	
	fDeltas.Allocate (size.Get ());
	
	DoZeroBytes (fDeltas.Buffer (), size.Get ());

	fRuntimeFingerprint.Clear ();

	}

/*****************************************************************************/

void dng_hue_sat_map::GetDelta (uint32 hueDiv,
								uint32 satDiv,
								uint32 valDiv,
								HSBModify &modify) const
	{

	if (hueDiv >= fHueDivisions ||
		satDiv >= fSatDivisions ||
		valDiv >= fValDivisions ||
		fDeltas.Buffer () == NULL)
		{
		
		DNG_REPORT ("Bad parameters to dng_hue_sat_map::GetDelta");
		
		ThrowProgramError ();
		
		}

	int32 offset = valDiv * fValStep +
				   hueDiv * fHueStep +
				   satDiv;

	const HSBModify *deltas = GetConstDeltas ();

	modify.fHueShift = deltas [offset].fHueShift;
	modify.fSatScale = deltas [offset].fSatScale;
	modify.fValScale = deltas [offset].fValScale;

	}

/*****************************************************************************/

void dng_hue_sat_map::SetDeltaKnownWriteable (uint32 hueDiv,
											  uint32 satDiv,
											  uint32 valDiv,
											  const HSBModify &modify)
	{

	if (hueDiv >= fHueDivisions ||
		satDiv >= fSatDivisions ||
		valDiv >= fValDivisions ||
		fDeltas.Buffer () == NULL)
		{
		
		DNG_REPORT ("Bad parameters to dng_hue_sat_map::SetDelta");
		
		ThrowProgramError ();
		
		}
		
	// Set this entry.
		
	int32 offset = valDiv * fValStep +
				   hueDiv * fHueStep +
				   satDiv;

	SafeGetDeltas () [offset] = modify;
	
	// The zero saturation entry is required to have a value scale
	// of 1.0f.
	
	if (satDiv == 0)
		{
		
		if (modify.fValScale != 1.0f)
			{
			
			#if qDNGValidate
		
			ReportWarning ("Value scale for zero saturation entries must be 1.0");
						 
			#endif
			
			SafeGetDeltas () [offset] . fValScale = 1.0f;
		
			}
		
		}
		
	// If we are settings the first saturation entry and we have not
	// set the zero saturation entry yet, fill in the zero saturation entry
	// by extrapolating first saturation entry.
	
	if (satDiv == 1)
		{
		
		HSBModify zeroSatModify;
		
		GetDelta (hueDiv, 0, valDiv, zeroSatModify);
		
		if (zeroSatModify.fValScale != 1.0f)
			{
			
			zeroSatModify.fHueShift = modify.fHueShift;
			zeroSatModify.fSatScale = modify.fSatScale;
			zeroSatModify.fValScale = 1.0f;
			
			SetDelta (hueDiv, 0, valDiv, zeroSatModify);
			
			}
		
		}

	}

/*****************************************************************************/

void dng_hue_sat_map::AssignNewUniqueRuntimeFingerprint ()
	{

	const uint64 uid = ++sRuntimeFingerprintCounter;

	dng_md5_printer printer;
	printer.Process (&uid, sizeof (uid));
	fRuntimeFingerprint = printer.Result ();

	}

/*****************************************************************************/

bool dng_hue_sat_map::operator== (const dng_hue_sat_map &rhs) const
	{
	
	if (fHueDivisions != rhs.fHueDivisions ||
		fSatDivisions != rhs.fSatDivisions ||
		fValDivisions != rhs.fValDivisions)
		return false;

	if (!IsValid ())
		return true;

	return memcmp (GetConstDeltas (),
				   rhs.GetConstDeltas (),
				   DeltasCount () * sizeof (HSBModify)) == 0;

	}

/*****************************************************************************/

dng_hue_sat_map * dng_hue_sat_map::Interpolate (const dng_hue_sat_map &map1,
											    const dng_hue_sat_map &map2,
											    real64 weight1)
	{
	
	if (weight1 >= 1.0)
		{
		
		if (!map1.IsValid ())
			{
			
			DNG_REPORT ("map1 is not valid");
			
			ThrowProgramError ();
			
			}
			
		return new dng_hue_sat_map (map1);
		
		}
		
	if (weight1 <= 0.0)
		{
		
		if (!map2.IsValid ())
			{			
			DNG_REPORT ("map2 is not valid");
			
			ThrowProgramError ();
			
			}
			
		return new dng_hue_sat_map (map2);
		
		}
		
	// Both maps must be valid if we are using both.
	
	if (!map1.IsValid () || !map2.IsValid ())
		{
			
		DNG_REPORT ("map1 or map2 is not valid");
		
		ThrowProgramError ();
		
		}
		
	// Must have the same dimensions.
	
	if (map1.fHueDivisions != map2.fHueDivisions ||
		map1.fSatDivisions != map2.fSatDivisions ||
		map1.fValDivisions != map2.fValDivisions)
		{
		
		DNG_REPORT ("map1 and map2 have different sizes");
		
		ThrowProgramError ();
		
		}
		
	// Make table to hold interpolated results.
	
	AutoPtr<dng_hue_sat_map> result (new dng_hue_sat_map);
	
	result->SetDivisions (map1.fHueDivisions,
						  map1.fSatDivisions,
						  map1.fValDivisions);
						  
	// Interpolate between the tables.
	
	real32 w1 = (real32) weight1;
	real32 w2 = 1.0f - w1;
	
	const HSBModify *data1 = map1.GetConstDeltas ();
	const HSBModify *data2 = map2.GetConstDeltas ();
	
	HSBModify *data3 = result->SafeGetDeltas ();
	
	uint32 count = map1.DeltasCount ();
	
	for (uint32 index = 0; index < count; index++)
		{
		
		data3->fHueShift = w1 * data1->fHueShift +
						   w2 * data2->fHueShift;
						   
		data3->fSatScale = w1 * data1->fSatScale +
						   w2 * data2->fSatScale;
						   
		data3->fValScale = w1 * data1->fValScale +
						   w2 * data2->fValScale;
						   
		data1++;
		data2++;
		data3++;
		
		}

	// Compute a fingerprint based on the inputs for the new dng_hue_sat_map
	// so that repeated interpolations of the same objects with the same
	// parameters produce the same fingerprint each time.

		{

		dng_md5_printer printer;

		printer.Process ("Interpolate", 11);

		printer.Process (&weight1, sizeof(weight1));

		printer.Process (map1.RuntimeFingerprint ().data,
						 dng_fingerprint::kDNGFingerprintSize);

		printer.Process (map2.RuntimeFingerprint ().data,
						 dng_fingerprint::kDNGFingerprintSize);

		result->SetRuntimeFingerprint (printer.Result ());

		}

	// Return interpolated tables.
	
	return result.Release ();
		
	}

/*****************************************************************************/
