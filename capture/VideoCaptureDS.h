#pragma once
#include "Comm.h"
#include "DeviceInfoDS.h"
#include "SinkFilterDS.h"
#include "helpfunc.h"
#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME "SinkFilter"


class VideoCaptureDS
{
public:
	VideoCaptureDS(const int32_t id, HWND wnd);
	~VideoCaptureDS();
	virtual int32_t Init(const int32_t id, const char* deviceUniqueIdUTF8, VideoCaptureObserver* observer);

	/*************************************************************************
	*
	*   Start/Stop
	*
	*************************************************************************/
	virtual int32_t
		StartCapture(const VideoCaptureCapability& capability);
	virtual int32_t StopCapture();

	/**************************************************************************
	*
	*   Properties of the set device
	*
	**************************************************************************/

	virtual bool CaptureStarted();
	virtual int32_t CaptureSettings(VideoCaptureCapability& settings);
private:
	int32_t SetCameraOutput(const VideoCaptureCapability& requestedCapability);
	int32_t DisconnectGraph();
	HRESULT ConnectDVCamera();

	DeviceInfoDS _dsInfo;

	IBaseFilter* _captureFilter;
	IGraphBuilder* _graphBuilder;
	IMediaControl* _mediaControl;
	//IVideoWindow *m_pVideoWindow;
	CaptureSinkFilter* _sinkFilter;
	IPin* _inputSendPin;
	IPin* _outputCapturePin;

	//IPin* _inputPreviewPin;
	//IBaseFilter *m_pVideoRenderer;
	// Microsoft DV interface (external DV cameras)
	IBaseFilter* _dvFilter;
	IPin* _inputDvPin;
	IPin* _outputDvPin;
	std::string _deviceUniqueId; // current Device unique name;
	int32_t _id;
	VideoCaptureCapability _requestedCapability;
	int32_t _captureDelay;
	HWND m_hVideoWindow;
};

