#include "stdafx.h"
#include "DeviceInfoDS.h"
#include "comutil.h"
#include "dvdmedia.h"
#include "helpfunc.h"
#include "strmif.h"
#include "logging.h"
#pragma comment(lib,"comsuppw.lib")
#pragma comment(lib, "sensorsapi.lib")
using namespace std;

//#define RELEASE_AND_CLEAR(p) if (p) { (p) -> Release () ; (p) = NULL ; }

void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// pUnk should not be used.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		_FreeMediaType(*pmt);
		CoTaskMemFree(pmt);
	}
}

DeviceInfoDS::DeviceInfoDS(const int32_t id)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
}


DeviceInfoDS::~DeviceInfoDS()
{
}

int32_t DeviceInfoDS::Init() {
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **)&_dsDevEnum);
	if (hr != NOERROR) {
		LOG(INFO) << " CoCreateInstance error ";
	}
	return 0;
}

int32_t DeviceInfoDS::GetDeviceInfo(
	uint32_t deviceNumber,
	std::string* deviceNameUTF8,
	std::string* deviceUniqueIdUTF8,
	std::string* productUniqueIdUTF8)

{
	// enumerate all video capture devices

	HRESULT hr =
		_dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
			&_dsMonikerDevEnum, 0);
	if (hr != NOERROR) {
		LOG(INFO) << " create CreateClassEnumerator error ";
		return 0;
	}

	_dsMonikerDevEnum->Reset();
	ULONG cFetched;
	IMoniker *pM;
	int index = 0;
	while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched)) {
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (S_OK == hr) {
			// Find the description or friendly name.
			VARIANT varName;
			VariantInit(&varName);
			hr = pBag->Read(L"Description", &varName, 0);
			if (FAILED(hr)) {
				hr = pBag->Read(L"FriendlyName", &varName, 0);
			}
			if (SUCCEEDED(hr)) {
				LOG(INFO) << " read var name  " << _com_util::ConvertBSTRToString(varName.bstrVal);
				// ignore all VFW drivers
				if ((wcsstr(varName.bstrVal, (L"(VFW)")) == NULL) &&
					(_wcsnicmp(varName.bstrVal, (L"Google Camera Adapter"), 21)
						!= 0)) {
					// Found a valid device.
					if (index == static_cast<int>(deviceNumber)) {
						int convResult = 0;

						//std::string str = (_bstr_t)(varName.bstrVal);
						*deviceNameUTF8 = _com_util::ConvertBSTRToString(varName.bstrVal);// WideToUTF8(varName.bstrVal);
						hr = pBag->Read(L"DevicePath", &varName, 0);
						if (FAILED(hr)) {
							*deviceUniqueIdUTF8 = *deviceNameUTF8;
							
						}
						else {
							*deviceUniqueIdUTF8 = _com_util::ConvertBSTRToString(varName.bstrVal);
							if (productUniqueIdUTF8) {
								GetProductId(deviceUniqueIdUTF8->c_str(),
									productUniqueIdUTF8);
							}
						}

					}
					++index; // increase the number of valid devices
				}
			}
			VariantClear(&varName);
			pBag->Release();
			pM->Release();
		}

	}
	LOG(INFO) << " The device name  " << *deviceNameUTF8  <<" unique id " << *deviceUniqueIdUTF8 << " index " <<index;
	//LOG_F(LS_INFO, VIDEO_CAPTURE_MODULE) << "The device name:" << *deviceNameUTF8;
	return index;
}
void DeviceInfoDS::GetProductId(const char* devicePath, std::string* productUniqueIdUTF8)
{
	*productUniqueIdUTF8 = '\0';
	char* startPos = strstr((char*)devicePath, "\\\\?\\");
	if (!startPos) {
		*productUniqueIdUTF8 = "";
		return;
	}
	startPos += 4;

	char* pos = strchr(startPos, '&');
	if (!pos || pos >= (char*)devicePath + strlen((char*)devicePath)) {
		*productUniqueIdUTF8 = "";
		
		return;
	}
	// Find the second occurrence.
	pos = strchr(pos + 1, '&');
	uint32_t bytesToCopy = (uint32_t)(pos - startPos);
	if (pos) {
		*productUniqueIdUTF8 = std::string(startPos, bytesToCopy);
	}
	else {
		*productUniqueIdUTF8 = "";

	}
}

IBaseFilter * DeviceInfoDS::GetDeviceFilter(
	const char* deviceUniqueIdUTF8,
	std::string* productUniqueIdUTF8)
{
	const int32_t deviceUniqueIdUTF8Length =
		(int32_t)strlen((char*)deviceUniqueIdUTF8); // UTF8 is also NULL terminated
	if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength) {
		LOG(INFO) << " EEEEEEEEEEEEEEE  ";
		return NULL;
	}

	// enumerate all video capture devices
	RELEASE_AND_CLEAR(_dsMonikerDevEnum);
	HRESULT hr = _dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&_dsMonikerDevEnum, 0);
	if (hr != NOERROR) {
		LOG(INFO) << " CreateClassEnumerator  error ";
		return 0;
	}
	_dsMonikerDevEnum->Reset();
	ULONG cFetched;
	IMoniker *pM;

	IBaseFilter *captureFilter = NULL;
	bool deviceFound = false;
	while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched) && !deviceFound) {
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (S_OK == hr) {
			// Find the description or friendly name.
			VARIANT varName;
			VariantInit(&varName);
			if (deviceUniqueIdUTF8Length > 0) {
				hr = pBag->Read(L"DevicePath", &varName, 0);
				if (FAILED(hr)) {
					hr = pBag->Read(L"Description", &varName, 0);
					if (FAILED(hr)) {
						hr = pBag->Read(L"FriendlyName", &varName, 0);
					}
				}
				LOG(INFO) << "GetDeviceFilter read var name  " << _com_util::ConvertBSTRToString(varName.bstrVal);
				if (SUCCEEDED(hr)) {
					char tempDevicePathUTF8[256];
					tempDevicePathUTF8[0] = 0;
					WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1,
						tempDevicePathUTF8,
						sizeof(tempDevicePathUTF8), NULL,
						NULL);
					if (strncmp(tempDevicePathUTF8,
						(const char*)deviceUniqueIdUTF8,
						deviceUniqueIdUTF8Length) == 0) {
						// We have found the requested device
						deviceFound = true;
						hr = pM->BindToObject(0, 0, IID_IBaseFilter,
							(void**)&captureFilter);
						if FAILED(hr) {
							
						}

						if (productUniqueIdUTF8) { // Get the device name
							GetProductId(deviceUniqueIdUTF8,
								productUniqueIdUTF8);
						}

					}
				}
			}
			VariantClear(&varName);
			pBag->Release();
			pM->Release();
		}
	}
	return captureFilter;
}

int32_t DeviceInfoDS::CreateCapabilityMap(
	const char* deviceUniqueIdUTF8)

{
	// Reset old capability list
	_captureCapabilities.clear();

	const int32_t deviceUniqueIdUTF8Length =
		(int32_t)strlen((char*)deviceUniqueIdUTF8);
	if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength) {

		return -1;
	}
	


	std::string productId;
	IBaseFilter* captureDevice = DeviceInfoDS::GetDeviceFilter(
		deviceUniqueIdUTF8,
		&productId);
	if (!captureDevice)
		return -1;
	IPin* outputCapturePin = GetOutputPin(captureDevice, GUID_NULL);
	if (!outputCapturePin) {
	
		return -1;
	}
	IAMExtDevice* extDevice = NULL;
	HRESULT hr = captureDevice->QueryInterface(IID_IAMExtDevice,
		(void **)&extDevice);
	if (SUCCEEDED(hr) && extDevice) {
	
		extDevice->Release();
	}

	IAMStreamConfig* streamConfig = NULL;
	hr = outputCapturePin->QueryInterface(IID_IAMStreamConfig,
		(void**)&streamConfig);
	if (FAILED(hr)) {
	
		return -1;
	}

	// this  gets the FPS
	IAMVideoControl* videoControlConfig = NULL;
	HRESULT hrVC = captureDevice->QueryInterface(IID_IAMVideoControl,
		(void**)&videoControlConfig);
	if (FAILED(hrVC)) {
		
	}

	AM_MEDIA_TYPE *pmt = NULL;
	VIDEO_STREAM_CONFIG_CAPS caps;
	int count, size;

	hr = streamConfig->GetNumberOfCapabilities(&count, &size);
	if (FAILED(hr)) {

		RELEASE_AND_CLEAR(videoControlConfig);
		RELEASE_AND_CLEAR(streamConfig);
		RELEASE_AND_CLEAR(outputCapturePin);
		RELEASE_AND_CLEAR(captureDevice);
		return -1;
	}

	// Check if the device support formattype == FORMAT_VideoInfo2 and FORMAT_VideoInfo.
	// Prefer FORMAT_VideoInfo since some cameras (ZureCam) has been seen having problem with MJPEG and FORMAT_VideoInfo2
	// Interlace flag is only supported in FORMAT_VideoInfo2
	bool supportFORMAT_VideoInfo2 = false;
	bool supportFORMAT_VideoInfo = false;
	bool foundInterlacedFormat = false;
	GUID preferedVideoFormat = FORMAT_VideoInfo;
	for (int32_t tmp = 0; tmp < count; ++tmp) {
		hr = streamConfig->GetStreamCaps(tmp, &pmt,
			reinterpret_cast<BYTE*> (&caps));
		if (!FAILED(hr)) {
			if (pmt->majortype == MEDIATYPE_Video
				&& pmt->formattype == FORMAT_VideoInfo2) {

				supportFORMAT_VideoInfo2 = true;
				VIDEOINFOHEADER2* h =
					reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
				//ASSERT(h);
				foundInterlacedFormat |= h->dwInterlaceFlags
					& (AMINTERLACE_IsInterlaced
						| AMINTERLACE_DisplayModeBobOnly);
			}
			if (pmt->majortype == MEDIATYPE_Video
				&& pmt->formattype == FORMAT_VideoInfo) {
			
				supportFORMAT_VideoInfo = true;
			}
		}
	}
	if (supportFORMAT_VideoInfo2) {
		if (supportFORMAT_VideoInfo && !foundInterlacedFormat) {
			preferedVideoFormat = FORMAT_VideoInfo;
		}
		else {
			preferedVideoFormat = FORMAT_VideoInfo2;
		}
	}

	for (int32_t tmp = 0; tmp < count; ++tmp) {
		hr = streamConfig->GetStreamCaps(tmp, &pmt,
			reinterpret_cast<BYTE*> (&caps));
		if (FAILED(hr)) {
		
			RELEASE_AND_CLEAR(videoControlConfig);
			RELEASE_AND_CLEAR(streamConfig);
			RELEASE_AND_CLEAR(outputCapturePin);
			RELEASE_AND_CLEAR(captureDevice);
			return -1;
		}

		if (pmt->majortype == MEDIATYPE_Video
			&& pmt->formattype == preferedVideoFormat) {

			VideoCaptureCapabilityWinDS capability;
			int64_t avgTimePerFrame = 0;

			if (pmt->formattype == FORMAT_VideoInfo) {
				VIDEOINFOHEADER* h =
					reinterpret_cast<VIDEOINFOHEADER*> (pmt->pbFormat);
	
				capability.directShowCapabilityIndex = tmp;
				capability.width = h->bmiHeader.biWidth;
				capability.height = h->bmiHeader.biHeight;
				avgTimePerFrame = h->AvgTimePerFrame;
			}
			if (pmt->formattype == FORMAT_VideoInfo2) {
				VIDEOINFOHEADER2* h =
					reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
				//ASSERT(h);
				capability.directShowCapabilityIndex = tmp;
				capability.width = h->bmiHeader.biWidth;
				capability.height = h->bmiHeader.biHeight;
				capability.interlaced = h->dwInterlaceFlags
					& (AMINTERLACE_IsInterlaced
						| AMINTERLACE_DisplayModeBobOnly);
				avgTimePerFrame = h->AvgTimePerFrame;
			}

			if (hrVC == S_OK) {
				LONGLONG *frameDurationList;
				LONGLONG maxFPS;
				long listSize;
				SIZE size;
				size.cx = capability.width;
				size.cy = capability.height;

				// GetMaxAvailableFrameRate doesn't return max frame rate always
				// eg: Logitech Notebook. This may be due to a bug in that API
				// because GetFrameRateList array is reversed in the above camera. So
				// a util method written. Can't assume the first value will return
				// the max fps.
				hrVC = videoControlConfig->GetFrameRateList(outputCapturePin,
					tmp, size,
					&listSize,
					&frameDurationList);

				// On some odd cameras, you may get a 0 for duration.
				// GetMaxOfFrameArray returns the lowest duration (highest FPS)
				if (hrVC == S_OK && listSize > 0 &&
					0 != (maxFPS = GetMaxOfFrameArray(frameDurationList,
						listSize))) {
					capability.maxFPS = static_cast<int> (10000000
						/ maxFPS);
					capability.supportFrameRateControl = true;
				}
				else { // use existing method

					if (avgTimePerFrame > 0)
						capability.maxFPS = static_cast<int> (10000000
							/ avgTimePerFrame);
					else
						capability.maxFPS = 0;
				}
			}
			else { // use existing method in case IAMVideoControl is not supported
				if (avgTimePerFrame > 0)
					capability.maxFPS = static_cast<int> (10000000
						/ avgTimePerFrame);
				else
					capability.maxFPS = 0;
			}

			// can't switch MEDIATYPE :~(
			
			if (pmt->subtype == MEDIASUBTYPE_I420) {
				capability.rawType = kVideoI420;
			}
			else if (pmt->subtype == MEDIASUBTYPE_IYUV) {
				capability.rawType = kVideoIYUV;
			}
			else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
				capability.rawType = kVideoRGB24;
			}
			else if (pmt->subtype == MEDIASUBTYPE_YUY2) {
				capability.rawType = kVideoYUY2;
			}
			else if (pmt->subtype == MEDIASUBTYPE_RGB565) {
				capability.rawType = kVideoRGB565;
			}
			else if (pmt->subtype == MEDIASUBTYPE_MJPG) {
				capability.rawType = kVideoMJPEG;
			}
			else if (pmt->subtype == MEDIASUBTYPE_dvsl
				|| pmt->subtype == MEDIASUBTYPE_dvsd
				|| pmt->subtype == MEDIASUBTYPE_dvhd) { // If this is an external DV camera
				capability.rawType = kVideoYUY2;// MS DV filter seems to create this type
			}
			else if (pmt->subtype == MEDIASUBTYPE_UYVY) { // Seen used by Declink capture cards
				capability.rawType = kVideoUYVY;
			}
			else if (pmt->subtype == MEDIASUBTYPE_HDYC) { // Seen used by Declink capture cards. Uses BT. 709 color. Not entiry correct to use UYVY. http://en.wikipedia.org/wiki/YCbCr
			
				capability.rawType = kVideoUYVY;
			}
			else {
				WCHAR strGuid[39];
				StringFromGUID2(pmt->subtype, strGuid, 39);
			
				continue;
			}

			// Get the expected capture delay from the static list
			//capability.expectedCaptureDelay
			//	= GetExpectedCaptureDelay(WindowsCaptureDelays,
			//		NoWindowsCaptureDelays,
			//		productId.c_str(),
			//		capability.width,
			//		capability.height);
			_captureCapabilities.push_back(capability);
			_captureCapabilitiesWindows.push_back(capability);
			LOG(INFO) << " width: " << capability.width
				<< " nheight:" << capability.height
				<< " codec type:" << capability.codecType
				<< " fps:" << capability.maxFPS
				<< " raw type:"<<capability.rawType;


		}
		_DeleteMediaType(pmt);
		pmt = NULL;
	}
	RELEASE_AND_CLEAR(streamConfig);
	RELEASE_AND_CLEAR(videoControlConfig);
	RELEASE_AND_CLEAR(outputCapturePin);
	RELEASE_AND_CLEAR(captureDevice); // Release the capture device

									  // Store the new used device name
	_lastUsedDeviceName = deviceUniqueIdUTF8;


	return static_cast<int32_t>(_captureCapabilities.size());
}

int32_t DeviceInfoDS::DisplayCaptureSettingsDialogBox(
	const char* deviceUniqueIdUTF8,
	const char* dialogTitleUTF8,
	void* parentWindow,
	uint32_t positionX,
	uint32_t positionY)
{
	HWND window = (HWND)parentWindow;

	IBaseFilter* filter = GetDeviceFilter(deviceUniqueIdUTF8, NULL);
	if (!filter)
		return -1;

	ISpecifyPropertyPages* pPages = NULL;
	CAUUID uuid;
	HRESULT hr = S_OK;

	hr = filter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID*)&pPages);
	if (!SUCCEEDED(hr)) {
		filter->Release();
		return -1;
	}
	hr = pPages->GetPages(&uuid);
	if (!SUCCEEDED(hr)) {
		filter->Release();
		return -1;
	}

	WCHAR tempDialogTitleWide[256];
	tempDialogTitleWide[0] = 0;
	int size = 255;

	// UTF-8 to wide char
	MultiByteToWideChar(CP_UTF8, 0, (char*)dialogTitleUTF8, -1,
		tempDialogTitleWide, size);

	// Invoke a dialog box to display.

	hr = OleCreatePropertyFrame(window, // You must create the parent window.
		positionX, // Horizontal position for the dialog box.
		positionY, // Vertical position for the dialog box.
		tempDialogTitleWide,// String used for the dialog box caption.
		1, // Number of pointers passed in pPlugin.
		(LPUNKNOWN*)&filter, // Pointer to the filter.
		uuid.cElems, // Number of property pages.
		uuid.pElems, // Array of property page CLSIDs.
		LOCALE_USER_DEFAULT, // Locale ID for the dialog box.
		0, NULL); // Reserved
				  // Release memory.
	if (uuid.pElems) {
		CoTaskMemFree(uuid.pElems);
	}
	filter->Release();
	return 0;
}

int32_t DeviceInfoDS::GetBestMatchedCapability(
	const char*deviceUniqueIdUTF8,
	const VideoCaptureCapability& requested,
	VideoCaptureCapability& resulting)
{

	if (!deviceUniqueIdUTF8) {
		LOG(ERROR) << "The device unique id is <NULL>." << std::endl;
		return -1;
	}

	if (_stricmp((char*)_lastUsedDeviceName.c_str(), (char*)deviceUniqueIdUTF8) != 0) {

		if (-1 == CreateCapabilityMap(deviceUniqueIdUTF8)) {
			LOG(ERROR) << "Create capability map failed.";
			return -1;
		}
	}

	int32_t bestformatIndex = -1;
	int32_t bestWidth = 0;
	int32_t bestHeight = 0;
	int32_t bestFrameRate = 0;
	RawVideoType bestRawType = kVideoUnknown;
	VideoCodecType bestCodecType = kVideoCodecUnknown;

	const int32_t numberOfCapabilies =
		static_cast<int32_t>(_captureCapabilities.size());

	for (int32_t tmp = 0;
		tmp < numberOfCapabilies;
		++tmp) { // Loop through all capabilities
		VideoCaptureCapability& capability = _captureCapabilities[tmp];
		LOG(INFO) << "capability.width:" << capability.width
			<< " capability.height:" << capability.height
			<< "capability.maxFPS:" << capability.maxFPS
			<< " requested.width:" << requested.width
			<< " requested.height:" << requested.height
			<< " requested.maxFPS:" << requested.maxFPS
			<< " currentBestWidth:" << bestWidth
			<< " currentBestHeight" << bestHeight
			<< " currentBestFrameRate:" << bestFrameRate
			<< " request.codecType:" << requested.codecType
			<< " capability.codecType:" << capability.codecType;

#ifndef ENABLE_MJPEG_CAPTURE
		if (capability.rawType == kVideoMJPEG) {
			LOG(INFO) << "Skip this format because it is MJPEG.";
			continue;
		}
#endif


		const int32_t diffWidth = capability.width - requested.width;
		const int32_t diffHeight = capability.height - requested.height;
		const int32_t diffFrameRate = capability.maxFPS - requested.maxFPS;

		const int32_t currentbestDiffWith = bestWidth - requested.width;
		const int32_t currentbestDiffHeight = bestHeight - requested.height;
		const int32_t currentbestDiffFrameRate = bestFrameRate - requested.maxFPS;

		if ((diffHeight >= 0 && diffHeight <= abs(currentbestDiffHeight)) // Height better or equalt that previouse.
			|| (currentbestDiffHeight < 0 && diffHeight >= currentbestDiffHeight)) {
			if (diffHeight == currentbestDiffHeight) { // Found best height. Care about the width)
				if ((diffWidth >= 0 && diffWidth <= abs(currentbestDiffWith)) // Width better or equal
					|| (currentbestDiffWith < 0 && diffWidth >= currentbestDiffWith)) {
					if (diffWidth == currentbestDiffWith && diffHeight
						== currentbestDiffHeight) { // Same size as previously
													//Also check the best frame rate if the diff is the same as previouse
						if (((diffFrameRate >= 0 &&
							diffFrameRate <= currentbestDiffFrameRate) // Frame rate to high but better match than previouse and we have not selected IUV
							||
							(currentbestDiffFrameRate < 0 &&
								diffFrameRate >= currentbestDiffFrameRate)) // Current frame rate is lower than requested. This is better.
							) {
							if ((currentbestDiffFrameRate == diffFrameRate) // Same frame rate as previous  or frame rate allready good enough
								|| (currentbestDiffFrameRate >= 0)) {
								if (bestRawType != requested.rawType
									&& requested.rawType != kVideoUnknown
									&& (capability.rawType == requested.rawType
										|| capability.rawType == kVideoI420
										|| capability.rawType == kVideoYUY2
										|| capability.rawType == kVideoYV12)) {
									bestCodecType = capability.codecType;
									bestRawType = capability.rawType;
									bestformatIndex = tmp;
								}
								// If width height and frame rate is full filled we can use the camera for encoding if it is supported.
								if (capability.height == requested.height
									&& capability.width == requested.width
									&& capability.maxFPS >= requested.maxFPS) {
									if (capability.codecType == requested.codecType
										&& bestCodecType != requested.codecType) {
										bestCodecType = capability.codecType;
										bestformatIndex = tmp;
									}
								}
							}
							else { // Better frame rate
								if (requested.codecType == capability.codecType) {

									bestWidth = capability.width;
									bestHeight = capability.height;
									bestFrameRate = capability.maxFPS;
									bestCodecType = capability.codecType;
									bestRawType = capability.rawType;
									bestformatIndex = tmp;
								}
							}
						}
					}
					else { // Better width than previously
						if (requested.codecType == capability.codecType) {
							bestWidth = capability.width;
							bestHeight = capability.height;
							bestFrameRate = capability.maxFPS;
							bestCodecType = capability.codecType;
							bestRawType = capability.rawType;
							bestformatIndex = tmp;
						}
					}
				}// else width no good
			}
			else { // Better height
				if (requested.codecType == capability.codecType) {
					bestWidth = capability.width;
					bestHeight = capability.height;
					bestFrameRate = capability.maxFPS;
					bestCodecType = capability.codecType;
					bestRawType = capability.rawType;
					bestformatIndex = tmp;
				}
			}
		}// else height not good
	}//end for

	LOG(INFO) << "Best camera format: " << bestWidth << "x" << bestHeight
		<< "@" << bestFrameRate
		<< "fps, color format: " << bestRawType;

	// Copy the capability
	if (bestformatIndex < 0) {
		LOG(INFO) << "Cannot find the best format index, so return -1.";
		return -1;
	}
	resulting = _captureCapabilities[bestformatIndex];
	return bestformatIndex;
}

int32_t DeviceInfoDS::GetWindowsCapability(
	const int32_t capabilityIndex,
	VideoCaptureCapabilityWinDS& windowsCapability)
{
	if (capabilityIndex < 0 || static_cast<size_t>(capabilityIndex) >=
		_captureCapabilitiesWindows.size()) {
		return -1;
	}

	windowsCapability = _captureCapabilitiesWindows[capabilityIndex];
	return 0;
}

uint32_t DeviceInfoDS::NumberOfDevices()
{
	std::string deviceName;
	std::string deviceUniqueId;
	std::string productId;
	return GetDeviceInfo(0, &deviceName, &deviceUniqueId, &productId);
}

int32_t DeviceInfoDS::GetDeviceName(
	uint32_t deviceNumber,
	std::string* deviceNameUTF8,
	std::string* deviceUniqueIdUTF8,
	std::string* productUniqueIdUTF8)
{
	const int32_t result = GetDeviceInfo(deviceNumber, deviceNameUTF8,
		deviceUniqueIdUTF8,
		productUniqueIdUTF8);
	return result > (int32_t)deviceNumber ? 0 : -1;
}

int32_t DeviceInfoDS::NumberOfCapabilities(
	const char* deviceUniqueIdUTF8)
{
	if (!deviceUniqueIdUTF8)
		return -1;

	if (_stricmp((char*)_lastUsedDeviceName.c_str(), deviceUniqueIdUTF8) == 0) {
		//yes
		return static_cast<int32_t>(_captureCapabilities.size());
	}

	int32_t ret = CreateCapabilityMap(deviceUniqueIdUTF8);
	return ret;
}