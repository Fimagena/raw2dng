/*****************************************************************************/
// Copyright 2006-2019 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

#ifndef __dng_point__
#define __dng_point__

/*****************************************************************************/

#include <cmath>

#include "dng_types.h"
#include "dng_utils.h"

/*****************************************************************************/

class dng_point
	{
	
	public:
	
		int32 v;
		int32 h;
	
	public:
	
		dng_point ()
			:	v (0)
			,	h (0)
			{
			}
			
		dng_point (int32 vv, int32 hh)
			:	v (vv)
			,	h (hh)
			{
			}
			
		bool operator== (const dng_point &pt) const
			{
			return (v == pt.v) &&
				   (h == pt.h);
			}
			
		bool operator!= (const dng_point &pt) const
			{
			return !(*this == pt);
			}

		real64 Length () const
			{
			return hypot ((real64) v, (real64) h);
			}
			
	};

/*****************************************************************************/

class dng_point_real64
	{
	
	public:
	
		real64 v;
		real64 h;
	
	public:
	
		dng_point_real64 ()
			:	v (0.0)
			,	h (0.0)
			{
			}
			
		dng_point_real64 (real64 vv, real64 hh)
			:	v (vv)
			,	h (hh)
			{
			}
			
		dng_point_real64 (const dng_point &pt)
			:	v ((real64) pt.v)
			,	h ((real64) pt.h)
			{
			}
			
		bool operator== (const dng_point_real64 &pt) const
			{
			return (v == pt.v) &&
				   (h == pt.h);
			}
			
		bool operator!= (const dng_point_real64 &pt) const
			{
			return !(*this == pt);
			}
			
		dng_point Round () const
			{
			return dng_point (Round_int32 (v),
							  Round_int32 (h));
			}

		real64 Length () const
			{
			return hypot (v, h);
			}

		void Scale (real64 scale)
			{
			v *= scale;
			h *= scale;
			}

		void Normalize ()
			{
			Scale (1.0 / Length ());
			}
			
	};

/*****************************************************************************/

inline dng_point operator+ (const dng_point &a,
				  			const dng_point &b)
				  
				  
	{
	
	return dng_point (a.v + b.v,
					  a.h + b.h);
					  
	}

/*****************************************************************************/

inline dng_point_real64 operator+ (const dng_point_real64 &a,
				  				   const dng_point_real64 &b)
				  
				  
	{
	
	return dng_point_real64 (a.v + b.v,
					  		 a.h + b.h);
					  
	}

/*****************************************************************************/

inline dng_point operator- (const dng_point &a,
				  			const dng_point &b)
				  
				  
	{
	
	return dng_point (a.v - b.v,
					  a.h - b.h);
					  
	}

/*****************************************************************************/

inline dng_point_real64 operator- (const dng_point_real64 &a,
				  				   const dng_point_real64 &b)
				  
				  
	{
	
	return dng_point_real64 (a.v - b.v,
					         a.h - b.h);
					  
	}

/*****************************************************************************/

inline real64 Distance (const dng_point_real64 &a,
						const dng_point_real64 &b)
				  
				  
	{

	return (a - b).Length ();
					  
	}

/*****************************************************************************/

inline real64 DistanceSquared (const dng_point_real64 &a,
							   const dng_point_real64 &b)
				  
				  
	{

	dng_point_real64 diff = a - b;

	return (diff.v * diff.v) + (diff.h * diff.h);
					  
	}

/*****************************************************************************/

// Finds distance squared from point p to line segment from v to w.

inline real64 DistanceSquared (const dng_point_real64 &p,
                               const dng_point_real64 &v,
                               const dng_point_real64 &w)
    {
    
    real64 len2 = DistanceSquared (v, w);
    
    if (len2 == 0.0)
        return DistanceSquared (p, v);
        
    real64 t = ((p.h - v.h) * (w.h - v.h) +
                (p.v - v.v) * (w.v - v.v)) / len2;
        
    if (t <= 0.0)
        return DistanceSquared (p, v);
    
    if (t >= 1.0)
        return DistanceSquared (p, w);
  
    dng_point_real64 z;
  
    z.h = v.h + t * (w.h - v.h);
    z.v = v.v + t * (w.v - v.v);
  
    return DistanceSquared (p, z);
     
    }

/*****************************************************************************/

inline dng_point Transpose (const dng_point &a)
	{
	
	return dng_point (a.h, a.v);
	
	}

/*****************************************************************************/

inline dng_point_real64 Transpose (const dng_point_real64 &a)
	{
	
	return dng_point_real64 (a.h, a.v);
	
	}

/*****************************************************************************/

inline dng_point_real64 Lerp (const dng_point_real64 &a,
							  const dng_point_real64 &b,
							  const real64 t)
	{
	
	return dng_point_real64 (Lerp_real64 (a.v, b.v, t),
							 Lerp_real64 (a.h, b.h, t));
	
	}

/*****************************************************************************/

inline real64 Dot (const dng_point_real64 &a,
				   const dng_point_real64 &b)
	{
	
	return (a.h * b.h) + (a.v * b.v);
	
	}

/*****************************************************************************/

inline dng_point_real64 operator* (const real64 scale,
								   const dng_point_real64 &pt)
	{
	
	dng_point_real64 result = pt;

	result.h *= scale;
	result.v *= scale;

	return result;
	
	}

/*****************************************************************************/

inline dng_point MakePerpendicular (const dng_point &pt)
	{
	
	return dng_point (-pt.h, pt.v);
	
	}

/*****************************************************************************/

inline dng_point_real64 MakePerpendicular (const dng_point_real64 &pt)
	{
	
	return dng_point_real64 (-pt.h, pt.v);
	
	}

/*****************************************************************************/

#endif
	
/*****************************************************************************/
