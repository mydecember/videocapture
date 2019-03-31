#pragma once
#include <DShow.h>
#include "stdint.h"
#include "string"
#include "vector"
#include "Comm.h"


class DeviceInfoDS
{
public:
	//static DeviceInfoDS* Create(const int32_t id);

	DeviceInfoDS(const int32_t id);
	virtual ~DeviceInfoDS();

	int32_t Init();
	uint32_t NumberOfDevices();

	int32_t
	GetDeviceName(uint32_t deviceNumber,
		std::string* deviceNameUTF8,
		std::string* deviceUniqueIdUTF8,
		std::string* productUniqueIdUTF8);
	int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);

	//virtual int32_t
	//	DisplayCaptureSettingsDialogBox(
	//		const char* deviceUniqueIdUTF8,
	//		const char* dialogTitleUTF8,
	//		void* parentWindow,
	//		uint32_t positionX,
	//		uint32_t positionY);

	//IBaseFilter * GetDeviceFilter(const char* deviceUniqueIdUTF8,
	//	std::string* productUniqueIdUTF8 = NULL);

	//int32_t
		//GetWindowsCapability(const int32_t capabilityIndex,
		//	VideoCaptureCapabilityWinDS& windowsCapability);

	//static void GetProductId(const char* devicePath,std::string* productUniqueIdUTF8);
	int32_t
		GetWindowsCapability(const int32_t capabilityIndex,
			VideoCaptureCapabilityWinDS& windowsCapability);
	virtual int32_t GetBestMatchedCapability(
		const char* deviceUniqueIdUTF8,
		const VideoCaptureCapability& requested,
		VideoCaptureCapability& resulting);
	int32_t GetDeviceInfo(uint32_t deviceNumber,
		std::string* deviceNameUTF8,
		std::string* deviceUniqueIdUTF8,
		std::string* productUniqueIdUTF8);
	void GetProductId(const char* devicePath, std::string* productUniqueIdUTF8);
	//virtual int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8);
	int32_t CreateCapabilityMap(
		const char* deviceUniqueIdUTF8);
	IBaseFilter * GetDeviceFilter(
		const char* deviceUniqueIdUTF8,
		std::string* productUniqueIdUTF8 =  NULL);
	int32_t
		DisplayCaptureSettingsDialogBox(
			const char* deviceUniqueIdUTF8,
			const char* dialogTitleUTF8,
			void* parentWindow,
			uint32_t positionX,
			uint32_t positionY);
private:
	ICreateDevEnum * _dsDevEnum;
	IEnumMoniker* _dsMonikerDevEnum;
	typedef std::vector<VideoCaptureCapability> VideoCaptureCapabilities;
	VideoCaptureCapabilities _captureCapabilities;
	std::vector<VideoCaptureCapabilityWinDS> _captureCapabilitiesWindows;
	std::string _lastUsedDeviceName;
};

