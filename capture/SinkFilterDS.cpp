#include "stdafx.h"
#include "SinkFilterDS.h"
#include "helpfunc.h"
#include <Dvdmedia.h> // VIDEOINFOHEADER2
#include <initguid.h>
#define kSecondsToReferenceTime 10000000

#define DELETE_RESET(p) { delete (p) ; (p) = NULL ;}

DEFINE_GUID(CLSID_SINKFILTER, 0x88cdbbdc, 0xa73b, 0x4afa, 0xac, 0xbf, 0x15, 0xd5,
	0xe2, 0xce, 0x12, 0xc3);
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;        // must be 0x1000
	LPCSTR szName;       // pointer to name (in user addr space)
	DWORD dwThreadID;    // thread ID (-1=caller thread)
	DWORD dwFlags;       // reserved for future use, must be zero
} THREADNAME_INFO;


CaptureInputPin::CaptureInputPin(CaptureSinkFilter* pFilter)
	: PinBase(pFilter),
	requested_capability_(),
	resulting_capability_()
{
	LOG(INFO) << " construct  CaptureInputPin " << this;
}

CaptureInputPin::~CaptureInputPin()
{
	LOG(INFO) << " ~~~ CaptureInputPin " << this;
}

bool CaptureInputPin::IsMediaTypeValid(const AM_MEDIA_TYPE* media_type) {

	GUID type = media_type->majortype;
	if (type != MEDIATYPE_Video) {
		LOG(ERROR) << "The type is not video, return false.";
		return false;
	}

	GUID format_type = media_type->formattype;
	if (format_type != FORMAT_VideoInfo) {
		LOG(ERROR) << "The format type is not VideoInfo, return false.";
		return false;
	}

	// Check for the sub types we support.
	GUID sub_type = media_type->subtype;
	VIDEOINFOHEADER* pvi =
		reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);
	if (pvi == NULL) {
		LOG(ERROR) << "The media type format is empty, return false.";
		return false;
	}

	// Store the incoming width and height.
	resulting_capability_.width = pvi->bmiHeader.biWidth;
	// For RGB24 the frame can be upside down.
	resulting_capability_.height = abs(pvi->bmiHeader.biHeight);

	LOG(INFO) << "IsMediaTypeValid width:" <<
		pvi->bmiHeader.biWidth << " height:" << pvi->bmiHeader.biHeight <<
		"Compression:" << std::hex << pvi->bmiHeader.biCompression;

	if (pvi->AvgTimePerFrame > 0) {
		resulting_capability_.maxFPS =
			static_cast<int>(kSecondsToReferenceTime / pvi->AvgTimePerFrame);
	}
	else {
		resulting_capability_.maxFPS = requested_capability_.maxFPS;
	}

	if (sub_type == MEDIASUBTYPE_I420
		&& pvi->bmiHeader.biCompression == MAKEFOURCC('I', '4', '2', '0')) {
		resulting_capability_.rawType = kVideoI420;
		LOG(INFO) << "Return true to mark I420 format acceptable.";
		return true; // This format is acceptable.
	}

	if (sub_type == MEDIASUBTYPE_YUY2
		&& pvi->bmiHeader.biCompression == MAKEFOURCC('Y', 'U', 'Y', '2')) {
		LOG(INFO) << "Return true to mark YUY2 format acceptable.";
		resulting_capability_.rawType = kVideoYUY2;
		return true; // This format is acceptable.
	}

	if (sub_type == MEDIASUBTYPE_UYVY
		&& pvi->bmiHeader.biCompression == MAKEFOURCC('U', 'Y', 'V', 'Y')) {
		resulting_capability_.rawType = kVideoUYVY;
		LOG(INFO) << "Return true to mark UYVY format acceptable.";
		return true; // This format is acceptable.
	}

	if (sub_type == MEDIASUBTYPE_HDYC) {
		resulting_capability_.rawType = kVideoUYVY;
		LOG(INFO) << "Return true to mark HDYC format acceptable.";
		return true; // This format is acceptable.
	}

	if (sub_type == MEDIASUBTYPE_RGB24
		&& pvi->bmiHeader.biCompression == BI_RGB) {
		resulting_capability_.rawType = kVideoRGB24;
		LOG(INFO) << "Return true to mark RGB24 format acceptable.";
		return true; // This format is acceptable.
	}


	if (sub_type == MEDIASUBTYPE_MJPG
		&& pvi->bmiHeader.biCompression == MAKEFOURCC('M', 'J', 'P', 'G')) {
		resulting_capability_.rawType = kVideoMJPEG;
		LOG(INFO) << "Return true to mark MJPG format acceptable.";
		return true; // This format is acceptable.
	}

	LOG(INFO) << "Return false to mark this format unacceptable.";
	return false;
}

bool CaptureInputPin::GetValidMediaType(int index, AM_MEDIA_TYPE* media_type) {
	LOG(INFO) << " index:" << index;
	if (media_type->cbFormat < sizeof(VIDEOINFOHEADER)) {
		LOG(ERROR) << "The cb format size:" << media_type->cbFormat << " sizeof(VIDEOINFOHEADER):" << sizeof(VIDEOINFOHEADER);
		return false;
	}

	VIDEOINFOHEADER* pvi =
		reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);

	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biClrImportant = 0;
	pvi->bmiHeader.biClrUsed = 0;
	if (requested_capability_.maxFPS > 0) {
		pvi->AvgTimePerFrame = kSecondsToReferenceTime /
			requested_capability_.maxFPS;
	}

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	media_type->majortype = MEDIATYPE_Video;
	media_type->formattype = FORMAT_VideoInfo;
	media_type->bTemporalCompression = FALSE;

	int32_t offset = 0;
	if (requested_capability_.codecType != kVideoCodecUnknown) {
		offset = 0;
	}
	LOG(INFO) << "index: " << index << " offset:" << offset << " codecType:" << requested_capability_.codecType;

	switch (index + offset) {
	case 0: {
		pvi->bmiHeader.biCompression = MAKEFOURCC('I', '4', '2', '0');
		pvi->bmiHeader.biBitCount = 12;  // bit per pixel
		pvi->bmiHeader.biWidth = requested_capability_.width;
		pvi->bmiHeader.biHeight = requested_capability_.height;
		pvi->bmiHeader.biSizeImage = 3 * requested_capability_.height *
			requested_capability_.width / 2;
		media_type->subtype = MEDIASUBTYPE_I420;
		break;
	}
	case 1: {
		pvi->bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
		pvi->bmiHeader.biBitCount = 16;
		pvi->bmiHeader.biWidth = requested_capability_.width;
		pvi->bmiHeader.biHeight = requested_capability_.height;
		pvi->bmiHeader.biSizeImage = 2 * requested_capability_.width *
			requested_capability_.height;
		media_type->subtype = MEDIASUBTYPE_YUY2;
		break;
	}
	case 2: {
		pvi->bmiHeader.biCompression = BI_RGB;
		pvi->bmiHeader.biBitCount = 24;
		pvi->bmiHeader.biWidth = requested_capability_.width;
		pvi->bmiHeader.biHeight = requested_capability_.height;
		pvi->bmiHeader.biSizeImage = 3 * requested_capability_.height *
			requested_capability_.width;
		media_type->subtype = MEDIASUBTYPE_RGB24;
		break;
	}

	case 3: {
		pvi->bmiHeader.biCompression = MAKEFOURCC('U', 'Y', 'V', 'Y');
		pvi->bmiHeader.biBitCount = 16; //bit per pixel
		pvi->bmiHeader.biWidth = requested_capability_.width;
		pvi->bmiHeader.biHeight = requested_capability_.height;
		pvi->bmiHeader.biSizeImage = 2 * requested_capability_.height
			*requested_capability_.width;
		media_type->subtype = MEDIASUBTYPE_UYVY;
	}
			break;
	case 4: {
		pvi->bmiHeader.biCompression = MAKEFOURCC('M', 'J', 'P', 'G');
		pvi->bmiHeader.biBitCount = 12; //bit per pixel
		pvi->bmiHeader.biWidth = requested_capability_.width;
		pvi->bmiHeader.biHeight = requested_capability_.height;
		pvi->bmiHeader.biSizeImage = 3 * requested_capability_.height
			* requested_capability_.width / 2;
		media_type->subtype = MEDIASUBTYPE_MJPG;
	}
			break;
	default:
		return false;
	}

	LOG(INFO) << "GetMediaType position " << index <<
		", width " << requested_capability_.width <<
		", height " << requested_capability_.height <<
		", biCompression " << std::hex << pvi->bmiHeader.biCompression;

	media_type->bFixedSizeSamples = TRUE;
	media_type->lSampleSize = pvi->bmiHeader.biSizeImage;
	return true;
}

HRESULT
CaptureInputPin::Receive(IN IMediaSample * sample)
{
	HRESULT hr = S_OK;

	IBaseFilter *owner = GetOwner();
	const int32_t length = sample->GetActualDataLength();
	unsigned char * buffer = NULL;
	if (FAILED(sample->GetPointer(&buffer)))
		return S_FALSE;

	reinterpret_cast <CaptureSinkFilter *>(owner)->ProcessCapturedFrame(buffer, length, resulting_capability_);
	return hr;
}

// called under LockReceive
HRESULT CaptureInputPin::SetMatchingMediaType(
	const VideoCaptureCapability& capability)
{

	requested_capability_ = capability;
	resulting_capability_ = VideoCaptureCapability();
	return S_OK;
}

////================================================>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


CaptureSinkFilter::CaptureSinkFilter(IN TCHAR * tszName,
	IN LPUNKNOWN punk,
	OUT HRESULT * phr,
	VideoCaptureObserver *captureObserver,
	int32_t moduleId)
	: m_pInput(NULL),
	captureObserver_(captureObserver)
{
	(*phr) = S_OK;
	m_pInput = new CaptureInputPin(this);
	return;
}

CaptureSinkFilter::~CaptureSinkFilter()
{
	LOG(INFO) << " ~~~ CaptureSinkFilter " << this;
	//delete m_pInput;
}

size_t CaptureSinkFilter::NoOfPins()
{
	return 1;
}

IPin* CaptureSinkFilter::GetPin(int index)
{
	LOG(INFO) << " to get pin " <<index << "  mpin " <<m_pInput ;
	if (index == 0) {
		return m_pInput;
	}
	return NULL;
	return (index == 0) ? m_pInput : NULL;
}

void CaptureSinkFilter::ProcessCapturedFrame(unsigned char* pBuffer, int32_t length, const VideoCaptureCapability& frameInfo)
{
	//LOG(INFO) << " get data " << length
	//			<< "width:" <<frameInfo.width
	//	<< "height:" << frameInfo.height
	//	<< "rawtype:" << frameInfo.rawType
	//	<< "maxfps:" << frameInfo.maxFPS;
	FILTER_STATE state = State_Stopped;
	//GetState(0, &state);
	captureObserver_->OnReceiveFrame(pBuffer, length, frameInfo);
	if (state == State_Running) {
		//  we have the receiver lock
		//_captureObserver.ProcessIncomingFrame(pBuffer, length, frameInfo);
	}
	return;
}

STDMETHODIMP CaptureSinkFilter::SetMatchingMediaType(
	const VideoCaptureCapability& capability)
{
	HRESULT hr;
	if (m_pInput) {
		hr = m_pInput->SetMatchingMediaType(capability);
	}
	else {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP CaptureSinkFilter::GetClassID(CLSID* class_id)
{
	(*class_id) = CLSID_SINKFILTER;
	return S_OK;
}
