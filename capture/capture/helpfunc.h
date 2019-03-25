#pragma once
#include <dshow.h>
#include "wmsdkidl.h"
#include <initguid.h>  // Must come before the help_functions_ds.h include so
#include <cguid.h>
DEFINE_GUID(MEDIASUBTYPE_I420, 0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00,
	0xAA, 0x00, 0x38, 0x9B, 0x71);
DEFINE_GUID(MEDIASUBTYPE_HDYC, 0x43594448, 0x0000, 0x0010, 0x80, 0x00, 0x00,
	0xAA, 0x00, 0x38, 0x9B, 0x71);

#define RELEASE_AND_CLEAR(p) if (p) { (p) -> Release () ; (p) = NULL ; }


LONGLONG GetMaxOfFrameArray(LONGLONG *maxFps, long size);

IPin* GetInputPin(IBaseFilter* filter);
IPin* GetOutputPin(IBaseFilter* filter, REFGUID Category);
BOOL PinMatchesCategory(IPin *pPin, REFGUID Category);


