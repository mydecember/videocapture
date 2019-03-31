#include "stdafx.h"
#include "VideoCaptureDS.h"
#include "Comm.h"
#include "dvdmedia.h"
VideoCaptureDS::VideoCaptureDS(const int32_t id, HWND wnd) : _dsInfo(id), _captureFilter(NULL),
_graphBuilder(NULL), _mediaControl(NULL), _sinkFilter(NULL),
_inputSendPin(NULL), _outputCapturePin(NULL), _dvFilter(NULL),
_inputDvPin(NULL), _outputDvPin(NULL), m_hVideoWindow(wnd)
{
}


VideoCaptureDS::~VideoCaptureDS()
{
	LOG(INFO) << "~~~~ VideoCaptureDS.";
	if (_mediaControl) {
		_mediaControl->Stop();
	}
	if (_graphBuilder) {
		if (_sinkFilter)
			_graphBuilder->RemoveFilter(_sinkFilter);
		if (_captureFilter)
			_graphBuilder->RemoveFilter(_captureFilter);
		if (_dvFilter)
			_graphBuilder->RemoveFilter(_dvFilter);
	}
	RELEASE_AND_CLEAR(_captureFilter); // release the capture device
	RELEASE_AND_CLEAR(_sinkFilter);
	RELEASE_AND_CLEAR(_dvFilter);

	RELEASE_AND_CLEAR(_mediaControl);
	RELEASE_AND_CLEAR(_inputSendPin);
	RELEASE_AND_CLEAR(_outputCapturePin);

	RELEASE_AND_CLEAR(_inputDvPin);
	RELEASE_AND_CLEAR(_outputDvPin);

	RELEASE_AND_CLEAR(_graphBuilder);
}

int32_t VideoCaptureDS::Init(const int32_t id, const char* deviceUniqueIdUTF8, VideoCaptureObserver* observer)
{
	// Store the device name
	_deviceUniqueId = deviceUniqueIdUTF8;

	if (_dsInfo.Init() != 0)
		return -1;

	_captureFilter = _dsInfo.GetDeviceFilter(deviceUniqueIdUTF8);
	if (!_captureFilter) {
		LOG(INFO) << "Failed to create capture filter.";
		return -1;
	}

	// Get the interface for DirectShow's GraphBuilder
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
		(void **)&_graphBuilder);
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to create graph builder.";
		return -1;
	}

	/*hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pVideoRenderer);
	if (FAILED(hr) || m_pVideoRenderer == NULL)
	{		
		LOG(INFO) << " InnerBuildGraph ,error 5 when create VideoRenderer." << std::endl;
		return hr;
	}*/


	/*hr = m_pVideoRenderer->QueryInterface(IID_IVideoWindow, (void**)&m_pVideoWindow);
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to m_pVideoWindow builder.";
		return -1;
	}

	_graphBuilder->AddFilter(m_pVideoRenderer, L"Video Renderer");

	_inputPreviewPin = GetOutputPin(_captureFilter, PIN_CATEGORY_CAPTURE);*/

	hr = _graphBuilder->QueryInterface(IID_IMediaControl,
		(void **)&_mediaControl);
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to create media control builder.";
		return -1;
	}
	hr = _graphBuilder->AddFilter(_captureFilter, CAPTURE_FILTER_NAME);
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to add the capture device to the graph.";
		return -1;
	}

	_outputCapturePin = GetOutputPin(_captureFilter, PIN_CATEGORY_CAPTURE);
	LOG(INFO) <<" @@@ _outputCapturePin " << _outputCapturePin;
	// Create the sink filte used for receiving Captured frames.
	std::string m_csFileName = std::string("hello");
	TCHAR wc[MAX_PATH];
	std::string ss = SINK_FILTER_NAME;
	_stprintf_s(wc, MAX_PATH, _T("%S"), ss.c_str());//%S¿í×Ö·û

	_sinkFilter = new CaptureSinkFilter(wc, NULL, &hr, observer, _id);
	if (hr != S_OK) {
		LOG(INFO) << "Failed to create send filter";
		return -1;
	}
	_sinkFilter->AddRef();

	hr = _graphBuilder->AddFilter(_sinkFilter, wc);
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to add the send filter to the graph.";
		return -1;
	}
	_inputSendPin = _sinkFilter->GetPin(0);//GetInputPin(_sinkFilter);

	// Temporary connect here.
	// This is done so that no one else can use the capture device.
	//if (SetCameraOutput(_requestedCapability) != 0) {
	//	return -1;
	//}
	//hr = _mediaControl->Pause();
	//if (FAILED(hr)) {
	//    LOG(LS_INFO, VIDEO_CAPTURE_MODULE) << "Failed to Pause the Capture device. Is it already occupied?" << std::hex << hr;
	//    return -1;
	//}

	


	LOG(INFO) << "Capture device '" << deviceUniqueIdUTF8 << "' initialized.";
	return 0;
}

int32_t VideoCaptureDS::StartCapture(
	const VideoCaptureCapability& capability)
{

	//m_pVideoWindow->put_Owner((OAHWND)m_hVideoWindow);
	//m_pVideoWindow->put_WindowStyle(WS_CHILD);
	//m_pVideoWindow->SetWindowPosition(0, 0,100,100);
	//m_pVideoWindow->put_Visible(OATRUE);


	if (capability != _requestedCapability) {
		//DisconnectGraph();

		if (SetCameraOutput(capability) != 0) {
			return -1;
		}
	}
	LOG(INFO) <<" ========= start to run";
	HRESULT hr = _mediaControl->Run();
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to start the Capture device:" << std::hex << hr;
		return -1;
	}
	return 0;
}
   
int32_t VideoCaptureDS::StopCapture()
       {      


	HRESULT hr = _mediaControl->Stop();
	if (FAILED(hr)) {
		LOG(INFO) << "Failed to stop the capture graph. " << std::hex << hr;
		return -1;
	}
	return 0;
}
bool VideoCaptureDS::CaptureStarted()
{
	OAFilterState state = 0;
	HRESULT hr = _mediaControl->GetState(1000, &state);
	if (hr != S_OK && hr != VFW_S_CANT_CUE) {
		LOG(INFO) << "Failed to get the CaptureStarted status";
	}
	LOG(INFO) << "CaptureStarted:" << state;
	return state == State_Running;

}
int32_t VideoCaptureDS::CaptureSettings(
	VideoCaptureCapability& settings)
{
	settings = _requestedCapability;
	return 0;
}

int32_t VideoCaptureDS::SetCameraOutput(
	const VideoCaptureCapability& requestedCapability)
{

	// Get the best matching capability
	VideoCaptureCapability capability;
	int32_t capabilityIndex;

	// Store the new requested size
	_requestedCapability = requestedCapability;
	// Match the requested capability with the supported.
	if ((capabilityIndex = _dsInfo.GetBestMatchedCapability(_deviceUniqueId.c_str(),
		_requestedCapability,
		capability)) < 0) {
		return -1;
	}
	//Reduce the frame rate if possible.
	if (capability.maxFPS > requestedCapability.maxFPS) {
		capability.maxFPS = requestedCapability.maxFPS;
	}
	else if (capability.maxFPS <= 0) {
		capability.maxFPS = 30;
	}
	// Store the new expected capture delay
	_captureDelay = capability.expectedCaptureDelay;

	// Convert it to the windows capability index since they are not nexessary
	// the same
	VideoCaptureCapabilityWinDS windowsCapability;
	if (_dsInfo.GetWindowsCapability(capabilityIndex, windowsCapability) != 0) {
		return -1;
	}

	IAMStreamConfig* streamConfig = NULL;
	AM_MEDIA_TYPE *pmt = NULL;
	VIDEO_STREAM_CONFIG_CAPS caps;

	HRESULT hr = _outputCapturePin->QueryInterface(IID_IAMStreamConfig,
		(void**)&streamConfig);
	if (hr) {
		LOG(INFO) << "Can't get the Capture format settings.";
		return -1;
	}

	//Get the windows capability from the capture device
	bool isDVCamera = false;
	hr = streamConfig->GetStreamCaps(
		windowsCapability.directShowCapabilityIndex,
		&pmt, reinterpret_cast<BYTE*> (&caps));
	if (!FAILED(hr)) {
		if (pmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* h =
				reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
			if (capability.maxFPS > 0
				&& windowsCapability.supportFrameRateControl) {
				h->AvgTimePerFrame = REFERENCE_TIME(10000000.0
					/ capability.maxFPS);
			}
		}
		else {
			VIDEOINFOHEADER* h = reinterpret_cast<VIDEOINFOHEADER*>
				(pmt->pbFormat);
			if (capability.maxFPS > 0
				&& windowsCapability.supportFrameRateControl) {
				h->AvgTimePerFrame = REFERENCE_TIME(10000000.0
					/ capability.maxFPS);
			}

		}

		// Set the sink filter to request this capability
		_sinkFilter->SetMatchingMediaType(capability);
		//Order the capture device to use this capability
		hr += streamConfig->SetFormat(pmt);

		//Check if this is a DV camera and we need to add MS DV Filter
		if (pmt->subtype == MEDIASUBTYPE_dvsl
			|| pmt->subtype == MEDIASUBTYPE_dvsd
			|| pmt->subtype == MEDIASUBTYPE_dvhd) {
			LOG(INFO) << " ===subtype is ds ";
			isDVCamera = true; // This is a DV camera. Use MS DV filter
		}
	}
	RELEASE_AND_CLEAR(streamConfig);

	if (FAILED(hr)) {
		LOG(INFO) << "Failed to set capture device output format";
		return -1;
	}




	if (isDVCamera) {
		hr = ConnectDVCamera();
	}
	else {
		LOG(INFO) << "#$$$$$to connect pins " << _outputCapturePin <<"   --> " << _inputSendPin;
		hr = _graphBuilder->ConnectDirect(_outputCapturePin, _inputSendPin,	NULL);
		LOG(INFO) << "#### not dv ";
	}
	if (hr != S_OK) {
		LOG(INFO) << "Failed to connect the Capture graph:" << std::hex << hr;
		return -1;
	}
	LOG(INFO) << "#$$$$$ end set camera output";
	return 0;
}

int32_t VideoCaptureDS::DisconnectGraph()
{
	HRESULT hr = _mediaControl->Stop();
	hr += _graphBuilder->Disconnect(_outputCapturePin);
	hr += _graphBuilder->Disconnect(_inputSendPin);

	//if the DV camera filter exist
	if (_dvFilter) {
		_graphBuilder->Disconnect(_inputDvPin);
		_graphBuilder->Disconnect(_outputDvPin);
	}
	if (hr != S_OK) {
		LOG(INFO) << "Failed to Stop the Capture device for reconfiguration:" << std::hex << hr;
		return -1;
	}
	return 0;
}
HRESULT VideoCaptureDS::ConnectDVCamera()
{
	HRESULT hr = S_OK;

	if (!_dvFilter) {
		hr = CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC,
			IID_IBaseFilter, (void **)&_dvFilter);
		if (hr != S_OK) {
			LOG(INFO) << "Failed to create the dv decoder:" << std::hex << hr;
			return hr;
		}
		hr = _graphBuilder->AddFilter(_dvFilter, L"VideoDecoderDV");
		if (hr != S_OK) {
			LOG(INFO) << "Failed to add the dv decoder to the graph:" << std::hex << hr;
			return hr;
		}
		_inputDvPin = GetInputPin(_dvFilter);
		if (_inputDvPin == NULL) {
			LOG(INFO) << "Failed to get input pin from DV decoder";
			return -1;
		}
		_outputDvPin = GetOutputPin(_dvFilter, GUID_NULL);
		if (_outputDvPin == NULL) {
			LOG(INFO) << "Failed to get output pin from DV decoder";
			return -1;
		}
	}
	hr = _graphBuilder->ConnectDirect(_outputCapturePin, _inputDvPin, NULL);
	if (hr != S_OK) {
		LOG(INFO) <<
			"Failed to connect capture device to the dv devoder: " << std::hex << hr;
		return hr;
	}

	hr = _graphBuilder->ConnectDirect(_outputDvPin, _inputSendPin, NULL);
	if (hr != S_OK) {
		if (hr == 0x80070004) {
			LOG(INFO) <<
				"Failed to connect the capture device, busy";
		}
		else {
			LOG(INFO) << "Failed to connect capture device to the send graph:" << std::hex << hr;
		}
		return hr;
	}
	return hr;
}

