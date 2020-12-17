/*****************************************************************************/
// Copyright 2006-2019 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying it.
/*****************************************************************************/

#include "dng_stream.h"

#include "dng_abort_sniffer.h"
#include "dng_auto_ptr.h"
#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_globals.h"
#include "dng_flags.h"
#include "dng_memory.h"
#include "dng_tag_types.h"
#include "dng_assertions.h"

/*****************************************************************************/

dng_stream::dng_stream (dng_abort_sniffer *sniffer,
						uint32 bufferSize,
						uint64 offsetInOriginalFile)

	:	fSwapBytes			  (false)
	,	fHaveLength			  (false)
	,	fLength				  (0)
	,	fOffsetInOriginalFile (offsetInOriginalFile)
	,	fPosition			  (0)
	,	fMemBlock			  ()
	,	fBuffer				  (NULL)
	,	fBufferSize			  (Max_uint32 (bufferSize, gDNGStreamBlockSize * 2))
	,	fBufferStart		  (0)
	,	fBufferEnd			  (0)
	,	fBufferLimit		  (bufferSize)
	,	fBufferDirty		  (false)
	,	fSniffer			  (sniffer)
	
	{
    
    fMemBlock.Reset (gDefaultDNGMemoryAllocator.Allocate (fBufferSize));
    
    fBuffer = fMemBlock->Buffer_uint8 ();
	
	}
		
/*****************************************************************************/

dng_stream::dng_stream (const void *data,
						uint32 count,
						uint64 offsetInOriginalFile)
						
	:	fSwapBytes			  (false)
	,	fHaveLength			  (true)
	,	fLength				  (count)
	,	fOffsetInOriginalFile (offsetInOriginalFile)
	,	fPosition			  (0)
	,	fMemBlock			  ()
	,	fBuffer				  ((uint8 *) data)
	,	fBufferSize			  (count)
	,	fBufferStart		  (0)
	,	fBufferEnd			  (count)
	,	fBufferLimit		  (count)
	,	fBufferDirty		  (false)
	,	fSniffer			  (NULL)
	
	{
	
	}
		
/*****************************************************************************/

dng_stream::~dng_stream ()
	{
	
	}
		
/*****************************************************************************/

uint64 dng_stream::DoGetLength ()
	{
	
	ThrowProgramError ();

	return 0;
	
	}
		
/*****************************************************************************/

void dng_stream::DoRead (void * /* data */,
						 uint32 /* count */,
						 uint64 /* offset */)
	{
	
	ThrowProgramError ();

	}
		
/*****************************************************************************/

void dng_stream::DoSetLength (uint64 /* length */)
	{
	
	ThrowProgramError ();

	}
		
/*****************************************************************************/

void dng_stream::DoWrite (const void * /* data */,
						  uint32 /* count */,
						  uint64 /* offset */)
	{
	
	ThrowProgramError ();

	}
		
/*****************************************************************************/

bool dng_stream::BigEndian () const
	{
	
	return fSwapBytes != (!!qDNGBigEndian);
	
	}
		
/*****************************************************************************/

void dng_stream::SetBigEndian (bool bigEndian)
	{
	
	fSwapBytes = (bigEndian != (!!qDNGBigEndian));
	
	}
		
/*****************************************************************************/

void dng_stream::SetBufferSize (dng_memory_allocator &allocator,
                                uint32 newBufferSize)
    {
    
    if (newBufferSize != fBufferSize &&
        newBufferSize >= gDNGStreamBlockSize * 2 &&
        !Data () &&
        !fBufferDirty)
        {
        
        try
            {
            
            fMemBlock.Reset (allocator.Allocate (newBufferSize));
            
            fBuffer = fMemBlock->Buffer_uint8 ();
            
            fBufferSize  = newBufferSize;
		
            fBufferStart = 0;
            fBufferEnd   = 0;
            fBufferLimit = newBufferSize;
            
            }
            
        catch (...)
            {
            
            }
        
        }
        
    }

/*****************************************************************************/

const void * dng_stream::Data () const
	{
	
	if (fBufferStart == 0 && fHaveLength && fBufferEnd == fLength)
		{
		
		return fBuffer;
		
		}
		
	return NULL;
	
	}
		
/*****************************************************************************/

dng_memory_block * dng_stream::AsMemoryBlock (dng_memory_allocator &allocator)
	{
	
	Flush ();
	
	uint64 len64 = Length ();
	
	if (len64 > 0xFFFFFFFF)
		{
		ThrowProgramError ();
		}
	
	uint32 len = (uint32) len64;
	
	AutoPtr<dng_memory_block> block (allocator.Allocate (len));
	
	if (len)
		{
	
		SetReadPosition (0);
		
		Get (block->Buffer (), len);
		
		}
		
	return block.Release ();
	
	}
		
/*****************************************************************************/

void dng_stream::SetReadPosition (uint64 offset)
	{
	
	fPosition = offset;
	
	if (fPosition > Length ())
		{
		
		ThrowEndOfFile ();

		}
	
	}

/*****************************************************************************/

uint64 dng_stream::OffsetInOriginalFile () const
	{
	
	return fOffsetInOriginalFile;
	
	}

/*****************************************************************************/

uint64 dng_stream::PositionInOriginalFile () const
	{
	
	if (fOffsetInOriginalFile == kDNGStreamInvalidOffset)
		return kDNGStreamInvalidOffset;
	
	return fOffsetInOriginalFile + Position ();
	
	}

/*****************************************************************************/

void dng_stream::Get (void *data, uint32 count, uint32 maxOverRead)
	{
	
	while (count)
		{
		
		// See if the request is totally inside buffer.
		
		if (fPosition >= fBufferStart && fPosition + count <= fBufferEnd)
			{
            
			memcpy (data,
                    fBuffer + (uint32) (fPosition - fBufferStart),
					count);
                
			fPosition += count;
			
			return;
			
			}
			
		// See if first part of request is inside buffer.
		
		if (fPosition >= fBufferStart && fPosition < fBufferEnd)
			{
			
			uint32 block = (uint32) (fBufferEnd - fPosition);
			
			memcpy (data,
                    fBuffer + (fPosition - fBufferStart),
					block);
			
			count -= block;
			
			data = (void *) (((char *) data) + block);
			
			fPosition += block;
			
			}
			
		// Flush buffer if dirty.
		
		Flush ();
		
		// Do large reads unbuffered.
		
		if (count > fBufferSize)
			{
			DNG_ASSERT(maxOverRead == 0, "Over-read of large size unexpected");
			if (fPosition + count > Length ())
				{
				
				ThrowEndOfFile ();
				
				}
				
			DoRead (data,
					count,
					fPosition);
					
			fPosition += count;
			
			return;
			
			}
			
		// Figure out new buffer range.
		
		fBufferStart = fPosition;
		
		if (fBufferSize >= gDNGStreamBlockSize)
			{
			
			// Align to a file block.
			
			fBufferStart &= (uint64) ~((int64) (gDNGStreamBlockSize - 1));
			
			}
		
		fBufferEnd = Min_uint64 (fBufferStart + fBufferSize, Length ());

		if ((fBufferEnd - fPosition) < maxOverRead)
			return; // ep, allow over-read requests
		else
		if (fBufferEnd <= fPosition)
			{
			
			ThrowEndOfFile ();

			}
			
		// Read data into buffer.
		
		dng_abort_sniffer::SniffForAbort (fSniffer);
		
		DoRead (fBuffer,
				(uint32) (fBufferEnd - fBufferStart),
				fBufferStart);
		
		}

	}
		
/*****************************************************************************/

void dng_stream::SetWritePosition (uint64 offset)
	{
	
	fPosition = offset;
	
	}
		
/*****************************************************************************/

void dng_stream::Flush ()
	{
	
	if (fBufferDirty)
		{
		
		dng_abort_sniffer::SniffForAbort (fSniffer);

		DoWrite (fBuffer,
				 (uint32) (fBufferEnd - fBufferStart),
				 fBufferStart);
				 
		fBufferStart = 0;
		fBufferEnd   = 0;
		fBufferLimit = fBufferSize;
		
		fBufferDirty = false;
		
		}

	}

/*****************************************************************************/

void dng_stream::SetLength (uint64 length)
	{
	
	Flush ();
	
	if (Length () != length)
		{
		
		DoSetLength (length);
		
		fLength = length;
		
		}
		
	}
		
/*****************************************************************************/

void dng_stream::Put (const void *data,
					  uint32 count)
	{
	
	// See if we can replace or append to the existing buffer.
	
	uint64 endPosition = fPosition + count;
	
	if (fBufferDirty                &&
		fPosition   >= fBufferStart &&
		fPosition   <= fBufferEnd   &&
		endPosition <= fBufferLimit)
		{
		
		memcpy (fBuffer + (uint32) (fPosition - fBufferStart),
                data,
				count);
				
		if (fBufferEnd < endPosition)
			fBufferEnd = endPosition;
		
		}
		
	// Else we need to write to the file.
		
	else
		{
        
        // Write initial part of the data to buffer, if possible.
        
        if (fBufferDirty &&
            fPosition >= fBufferStart &&
            fPosition <= fBufferEnd   &&
            fPosition <  fBufferLimit)
            {
            
            uint32 subCount = (uint32) (fBufferLimit - fPosition);
            
            memcpy (fBuffer + (uint32) (fPosition - fBufferStart),
                    data,
                    subCount);
                
            count -= subCount;
            data   = (const void *) (((const uint8 *) data) + subCount);
            
            fPosition  = fBufferLimit;
            fBufferEnd = fBufferLimit;
            
            }

		// Write existing buffer.
		
		Flush ();
        
        // Figure out how much space we have in buffer from
        // current position to end of file block.
        
        uint64 blockRound = gDNGStreamBlockSize - 1;
        
        uint64 blockMask = ~((int64) blockRound);

        uint32 alignedSize = (uint32)
                             (((fPosition + fBufferSize) & blockMask) - fPosition);
            
		// If write request will not fit in buffer, then write everything except
        // for the final unaligned part of the data.
		
		if (count > alignedSize)
			{
            
            uint32 alignedCount = (uint32)
                                  (((fPosition + count) & blockMask) - fPosition);
			
			dng_abort_sniffer::SniffForAbort (fSniffer);
			
			DoWrite (data, alignedCount, fPosition);
			
            count -= alignedCount;
            data   = (const void *) (((const uint8 *) data) + alignedCount);

            fPosition += alignedCount;
            
			}
			
		// Start a new buffer with small blocks.
			
		if (count > 0)
			{
			
			fBufferDirty = true;
			
			fBufferStart = fPosition;
			fBufferEnd   = endPosition;
			fBufferLimit = (fBufferStart + fBufferSize) & blockMask;
			
			memcpy (fBuffer,
                    data,
					count);
				
			}
		
		}
		
	fPosition = endPosition;
	
	fLength = Max_uint64 (Length (), fPosition);
	
	}
		
/*****************************************************************************/

uint16 dng_stream::Get_uint16 ()
	{
	
	uint16 x;
	
	Get (&x, 2);
	
	if (fSwapBytes)
		{
		
		x = SwapBytes16 (x);
			
		}
	
	return x;
	
	}

/*****************************************************************************/

void dng_stream::Put_uint16 (uint16 x)
	{
	
	if (fSwapBytes)
		{
		
		x = SwapBytes16 (x);
			
		}
		
	Put (&x, 2);
	
	}

/*****************************************************************************/
uint32 dng_stream::Get_uint32 ()
	{
	
	uint32 x;
	
	Get (&x, 4);
	
	if (fSwapBytes)
		{
		
		x = SwapBytes32 (x);
			
		}
	
	return x;
	
	}

/*****************************************************************************/

void dng_stream::Put_uint32 (uint32 x)
	{
	
	if (fSwapBytes)
		{
		
		x = SwapBytes32 (x);
			
		}
		
	Put (&x, 4);
	
	}

/*****************************************************************************/

uint64 dng_stream::Get_uint64 ()
	{
	
	if (fSwapBytes)
		{
	
		union
			{
			uint32 u32 [2];
			uint64 u64;
			} u;
			
		u.u32 [1] = Get_uint32 ();
		u.u32 [0] = Get_uint32 ();
			
		return u.u64;
		
		}
		
	uint64 x;
	
	Get (&x, 8);
	
	return x;
	
	}

/*****************************************************************************/

void dng_stream::Put_uint64 (uint64 x)
	{
	
	if (fSwapBytes)
		{
		
		union
			{
			uint32 u32 [2];
			uint64 u64;
			} u;
			
		u.u64 = x;
			
		Put_uint32 (u.u32 [1]);
		Put_uint32 (u.u32 [0]);
			
		}
		
	else
		{
		
		Put (&x, 8);
		
		}
		
	}

/*****************************************************************************/

real32 dng_stream::Get_real32 ()
	{
	
	union
		{
		uint32 i;
		real32 r;
		} u;
		
	u.i = Get_uint32 ();
	
	return u.r;
	
	}

/*****************************************************************************/

void dng_stream::Put_real32 (real32 x)
	{
	
	if (fSwapBytes)
		{
	
		union
			{
			uint32 i;
			real32 r;
			} u;
			
		u.r = x;
	
		Put_uint32 (u.i);
		
		}
		
	else
		{
		
		Put (&x, 4);
		
		}
	
	}

/*****************************************************************************/

real64 dng_stream::Get_real64 ()
	{
	
	if (fSwapBytes)
		{
	
		union
			{
			uint32 i [2];
			real64 r;
			} u;
			
		u.i [1] = Get_uint32 ();
		u.i [0] = Get_uint32 ();
			
		return u.r;
		
		}
		
	real64 x;
	
	Get (&x, 8);
	
	return x;
	
	}

/*****************************************************************************/

void dng_stream::Put_real64 (real64 x)
	{
	
	if (fSwapBytes)
		{
		
		union
			{
			uint32 i [2];
			real64 r;
			} u;
			
		u.r = x;
			
		Put_uint32 (u.i [1]);
		Put_uint32 (u.i [0]);
			
		}
		
	else
		{
		
		Put (&x, 8);
		
		}
		
	}

/*****************************************************************************/
	
void dng_stream::Get_CString (char *data, uint32 maxLength)
	{

	memset (data, 0, maxLength);
	
	uint32 index = 0;
	
	while (true)
		{
		
		char c = (char) Get_uint8 ();
		
		if (index + 1 < maxLength)
			data [index++] = c;
		
		if (c == 0)
			break;
			
		}
	
	}

/*****************************************************************************/
	
void dng_stream::Get_UString (char *data, uint32 maxLength)
	{
	
	memset (data, 0, maxLength);
	
	uint32 index = 0;
	
	while (true)
		{
		
		char c = (char) Get_uint16 ();
		
		if (index + 1 < maxLength)
			data [index++] = (char) c;
		
		if (c == 0)
			break;
			
		}
	
	}
		
/*****************************************************************************/

void dng_stream::PutZeros (uint64 count)
	{
	
	const uint32 kZeroBufferSize = 4096;
	
	if (count >= kZeroBufferSize)
		{
		
		dng_memory_data zeroBuffer (kZeroBufferSize);
		
		DoZeroBytes (zeroBuffer.Buffer (), 
					 kZeroBufferSize);
		
		while (count)
			{
			
			uint64 blockSize = Min_uint64 (count, kZeroBufferSize);
			
			Put (zeroBuffer.Buffer (), (uint32) blockSize);
			
			count -= blockSize;
			
			}
		
		}
		
	else
		{
		
		uint32 count32 = (uint32) count;
	
		for (uint32 j = 0; j < count32; j++)
			{
			
			Put_uint8 (0);
			
			}
			
		}
	
	}
		
/*****************************************************************************/

void dng_stream::PadAlign2 ()
	{
	
	PutZeros (Position () & 1);
	
	}
		
/*****************************************************************************/

void dng_stream::PadAlign4 ()
	{
	
	PutZeros ((4 - (Position () & 3)) & 3);
	
	}

/*****************************************************************************/

uint32 dng_stream::TagValue_uint32 (uint32 tagType)
	{
	
	switch (tagType)
		{
		
		case ttByte:
			return (uint32) Get_uint8 ();
			
		case ttShort:
			return (uint32) Get_uint16 ();
			
		case ttLong:
		case ttIFD:
			return Get_uint32 ();
		
		}
		
	real64 x = TagValue_real64 (tagType);
	
	if (x < 0.0)
		x = 0.0;
		
	if (x > (real64) 0xFFFFFFFF)
		x = (real64) 0xFFFFFFFF;
		
	return (uint32) (x + 0.5);
	
	}
	
/*****************************************************************************/

int32 dng_stream::TagValue_int32 (uint32 tagType)
	{
	
	switch (tagType)
		{
		
		case ttSByte:
			return (int32) Get_int8 ();
			
		case ttSShort:
			return (int32) Get_int16 ();
			
		case ttSLong:
			return Get_int32 ();
		
		}
		
	real64 x = TagValue_real64 (tagType);
	
	if (x < 0.0)
		{
		
		if (x < -2147483648.0)
			x = -2147483648.0;
			
		return (int32) (x - 0.5);
		
		}
		
	else
		{
		
		if (x > 2147483647.0)
			x = 2147483647.0;
		
		return (int32) (x + 0.5);
		
		}
		
	}
	
/*****************************************************************************/

dng_urational dng_stream::TagValue_urational (uint32 tagType)
	{
	
	dng_urational result;
	
	result.n = 0;
	result.d = 1;
	
	switch (tagType)
		{
		
		case ttRational:
			{
		
			result.n = Get_uint32 ();
			result.d = Get_uint32 ();
			
			break;
			
			}
			
		case ttSRational:
			{
			
			int32 n = Get_int32 ();
			int32 d = Get_int32 ();
			
			if ((n < 0) == (d < 0))
				{
			
				if (d < 0)
					{
					n = -n;
					d = -d;
					}
					
				result.n = (uint32) n;
				result.d = (uint32) d;
					
				}
				
			break;
			
			}
			
		case ttByte:
		case ttShort:
		case ttLong:
		case ttIFD:
			{
			
			result.n = TagValue_uint32 (tagType);
			
			break;
			
			}
		
		case ttSByte:
		case ttSShort:
		case ttSLong:
			{
			
			int32 n = TagValue_int32 (tagType);
			
			if (n > 0)
				{
				result.n = (uint32) n;
				}
			
			break;
			
			}
			
		default:
			{
			
			real64 x = TagValue_real64 (tagType);
			
			if (x > 0.0)
				{
				
				while (result.d < 10000 && x < 1000000)
					{
					
					result.d *= 10;
					
					x *= 10.0;
					
					}
				
				result.n = (uint32) (x + 0.5);
				
				}
			
			}
		
		}
		
	return result;
		
	}

/*****************************************************************************/

dng_srational dng_stream::TagValue_srational (uint32 tagType)
	{
	
	dng_srational result;
	
	result.n = 0;
	result.d = 1;
	
	switch (tagType)
		{
		
		case ttSRational:
			{
		
			result.n = Get_int32 ();
			result.d = Get_int32 ();
			
			break;
			
			}
		
		default:
			{
			
			real64 x = TagValue_real64 (tagType);
			
			if (x > 0.0)
				{
				
				while (result.d < 10000 && x < 1000000.0)
					{
					
					result.d *= 10;
					
					x *= 10.0;
					
					}
				
				result.n = (int32) (x + 0.5);
				
				}
				
			else
				{
				
				while (result.d < 10000 && x > -1000000.0)
					{
					
					result.d *= 10;
					
					x *= 10.0;
					
					}
				
				result.n = (int32) (x - 0.5);
				
				}
			
			}
			
		}
		
	return result;

	}

/*****************************************************************************/

real64 dng_stream::TagValue_real64 (uint32 tagType)
	{
	
	switch (tagType)
		{
		
		case ttByte:
		case ttShort:
		case ttLong:
		case ttIFD:
			return (real64) TagValue_uint32 (tagType);
			
		case ttSByte:
		case ttSShort:
		case ttSLong:
			return (real64) TagValue_int32 (tagType);
			
		case ttRational:
			{
			
			uint32 n = Get_uint32 ();
			uint32 d = Get_uint32 ();
			
			if (d == 0)
				return 0.0;
			else
				return (real64) n / (real64) d;
				
			}
		
		case ttSRational:
			{
			
			int32 n = Get_int32 ();
			int32 d = Get_int32 ();
			
			if (d == 0)
				return 0.0;
			else
				return (real64) n / (real64) d;
				
			}
			
		case ttFloat:
			return (real64) Get_real32 ();
			
		case ttDouble:
			return Get_real64 ();
			
		}
		
	return 0.0;

	}
		
/*****************************************************************************/

void dng_stream::CopyToStream (dng_stream &dstStream,
							   uint64 count)
	{
	
	uint8 smallBuffer [1024];
		
	if (count <= sizeof (smallBuffer))
		{
		
		Get (smallBuffer, (uint32) count);
		
		dstStream.Put (smallBuffer, (uint32) count);
		
		}
		
	else
		{
	
		const uint32 bigBufferSize = (uint32) Min_uint64 (kBigBufferSize,
													      count);
		
		dng_memory_data bigBuffer (bigBufferSize);
		
		while (count)
			{
			
			uint32 blockCount = (uint32) Min_uint64 (bigBufferSize,
													 count);
													 
			Get (bigBuffer.Buffer (),
				 blockCount);
			
			dstStream.Put (bigBuffer.Buffer (),
						   blockCount);
						   
			count -= blockCount;
			
			}
	
		}

	}
		
/*****************************************************************************/

void dng_stream::DuplicateStream (dng_stream &dstStream)
	{
	
	// Turn off sniffers for this operation.
	
	TempStreamSniffer noSniffer1 (*this    , NULL);
	TempStreamSniffer noSniffer2 (dstStream, NULL);
		
	// First grow the destination stream if required, in an attempt to
	// reserve any needed space before overwriting the existing data.
	
	if (dstStream.Length () < Length ())
		{
		dstStream.SetLength (Length ());
		}
		
	SetReadPosition (0);
	
	dstStream.SetWritePosition (0);
	
	CopyToStream (dstStream, Length ());
	
	dstStream.Flush ();
							
	dstStream.SetLength (Length ());

	}

/*****************************************************************************/

dng_stream_contiguous_read_hint::dng_stream_contiguous_read_hint
                                 (dng_stream &stream,
                                  dng_memory_allocator &allocator,
                                  uint64 offset,
                                  uint64 count)

    :   fStream        (stream)
    ,   fAllocator     (allocator)
    ,   fOldBufferSize (stream.BufferSize ())

    {
    
    fStream.Flush ();       // Cannot change buffer size with dirty buffer
    
    // Don't bother changing buffer size if only a small change.
    
    if (count > fOldBufferSize * 4)
        {
        
        // Round contiguous size up and down to stream blocks.
        
        uint64 blockRound = gDNGStreamBlockSize - 1;
        
        uint64 blockMask  = ~((int64) blockRound);
        
        count = (count + (offset & blockRound) + blockRound) & blockMask;
        
        // Limit to maximum buffer size.
        
        uint64 newBufferSize = Min_uint64 (gDNGMaxStreamBufferSize, count);
        
        // To avoid reading too many bytes with the final read, adjust buffer
        // size the to make an exact number of buffers fit.
        
        uint64 numBuffers = (count + newBufferSize - 1) / newBufferSize;
        
        newBufferSize = (count + numBuffers - 1) / numBuffers;
        
        // Finally round up to a block size.
        
        newBufferSize = (newBufferSize + blockRound) & blockMask;
        
        // Change the buffer size.
        
        fStream.SetBufferSize (fAllocator, (uint32) newBufferSize);
        
        }
    
    }

/*****************************************************************************/

dng_stream_contiguous_read_hint::~dng_stream_contiguous_read_hint ()
    {
    
    fStream.SetBufferSize (fAllocator, fOldBufferSize);
    
    }

/*****************************************************************************/

TempBigEndian::TempBigEndian (dng_stream &stream,
						 	  bool bigEndian)
	
	:	fStream  (stream)
	,	fOldSwap (stream.SwapBytes ())
	
	{
	
	fStream.SetBigEndian (bigEndian);
	
	}
		
/*****************************************************************************/

TempBigEndian::~TempBigEndian ()
	{
	
	fStream.SetSwapBytes (fOldSwap);
	
	}
		
/*****************************************************************************/

TempStreamSniffer::TempStreamSniffer (dng_stream &stream,
									  dng_abort_sniffer *sniffer)
	
	:	fStream     (stream)
	,	fOldSniffer (stream.Sniffer ())
	
	{
	
	fStream.SetSniffer (sniffer);
	
	}
		
/*****************************************************************************/

TempStreamSniffer::~TempStreamSniffer ()
	{
	
	fStream.SetSniffer (fOldSniffer);
	
	}
		
/*****************************************************************************/
