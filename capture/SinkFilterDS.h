#pragma once
#include <dshow.h>
#include "Comm.h"
#include"PinBase.h"
#include "FilterBase.h"
class CaptureSinkFilter;

class CaptureInputPin :public PinBase
{
public:
	VideoCaptureCapability requested_capability_;
	VideoCaptureCapability resulting_capability_;
	HANDLE _threadHandle;

	CaptureInputPin(CaptureSinkFilter* pFilter);
	virtual ~CaptureInputPin();

	virtual bool IsMediaTypeValid(const AM_MEDIA_TYPE* media_type);

	// Enumerates valid media types.
	virtual bool GetValidMediaType(int index, AM_MEDIA_TYPE* media_type);

	// Called when new media is received. Note that this is not on the same
	// thread as where the pin is created.
	STDMETHOD(Receive)(IMediaSample* sample);

	HRESULT SetMatchingMediaType(const VideoCaptureCapability& capability);
private:
	//CaptureSinkFilter * pFilter_;
};

class CaptureSinkFilter: public FilterBase
{
public:
	CaptureSinkFilter(IN TCHAR * tszName,
		IN LPUNKNOWN punk,
		OUT HRESULT * phr,
		VideoCaptureObserver* captureObserver,
		int32_t moduleId);
	virtual ~CaptureSinkFilter();

	//Member of FilterBase
	virtual size_t NoOfPins();
	virtual IPin* GetPin(int index);
	STDMETHODIMP GetClassID(CLSID* class_id);

	//  --------------------------------------------------------------------
	//  class methods
	void ProcessCapturedFrame(unsigned char* pBuffer, int32_t length,
		const VideoCaptureCapability& frameInfo);

	STDMETHODIMP SetMatchingMediaType(const VideoCaptureCapability& capability);
private:
	CaptureInputPin * m_pInput;
	//VideoCaptureImpl& _captureObserver;
	VideoCaptureObserver* captureObserver_;
};

