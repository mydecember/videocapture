#include "stdafx.h"
#include "Comm.h"




AVPixelFormat RawTypeToAVFormat(RawVideoType type) {
	switch (type) {
	case kVideoI420:
		return AV_PIX_FMT_YUV420P;
	case kVideoYV12:
		return AV_PIX_FMT_NONE;
	case kVideoYUY2:
		return AV_PIX_FMT_YUYV422;
	case kVideoUYVY:
	case kVideoIYUV:
	case kVideoARGB:
	case kVideoRGB24:
	case kVideoRGB565:
	case kVideoARGB4444:
	case kVideoARGB1555:
	case kVideoMJPEG:
	case kVideoNV12:
	case kVideoNV21:
	case kVideoBGRA:
	case kVideoUnknown:
		break;
	}
	return AV_PIX_FMT_NONE;
}

Comm::Comm()
{
}


Comm::~Comm()
{
}
