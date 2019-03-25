#include "stdafx.h"
#include "helpfunc.h"
#include "logging.h"

// This returns minimum :), which will give max frame rate...
LONGLONG GetMaxOfFrameArray(LONGLONG *maxFps, long size)
{
	LONGLONG maxFPS = maxFps[0];
	for (int i = 0; i < size; i++) {
		if (maxFPS > maxFps[i])
			maxFPS = maxFps[i];
	}
	return maxFPS;
}

IPin* GetInputPin(IBaseFilter* filter)
{
	LOG(INFO) <<" GetInputPin ";
	HRESULT hr;
	IPin* pin = NULL;
	IEnumPins* pPinEnum = NULL;
	filter->EnumPins(&pPinEnum);
	if (pPinEnum == NULL) {
		return NULL;
	}

	// get first unconnected pin
	hr = pPinEnum->Reset(); // set to first pin
	LOG(INFO) << " GetInputPin  ppinEnum reset";
	while (S_OK == pPinEnum->Next(1, &pin, NULL)) {
		LOG(INFO) << " GetInputPin  ppinEnum 111111";
		PIN_DIRECTION pPinDir;
		pin->QueryDirection(&pPinDir);
		if (PINDIR_INPUT == pPinDir) { // This is an input pin
			IPin* tempPin = NULL;
			if (S_OK != pin->ConnectedTo(&tempPin)) { // The pint is not connected
				//pPinEnum->Release();
				return pin;
			}
		}
		//pin->Release();
	}
	//pPinEnum->Release();
	return NULL;
}

IPin* GetOutputPin(IBaseFilter* filter, REFGUID Category)
{
	HRESULT hr;
	IPin* pin = NULL;
	IEnumPins* pPinEnum = NULL;
	filter->EnumPins(&pPinEnum);
	if (pPinEnum == NULL) {
		return NULL;
	}
	// get first unconnected pin
	hr = pPinEnum->Reset(); // set to first pin
	LOG(INFO) << " GetOutputPin  ppinEnum reset";
	while (S_OK == pPinEnum->Next(1, &pin, NULL)) {
		PIN_DIRECTION pPinDir;
		pin->QueryDirection(&pPinDir);
		if (PINDIR_OUTPUT == pPinDir) { // This is an output pin
			if (Category == GUID_NULL || PinMatchesCategory(pin, Category)) {
				LOG(INFO) << " GetOutputPin  pPinEnum->Release()";
				pPinEnum->Release();
				return pin;
			}
		}
		LOG(INFO) << " GetOutputPin  pin->Release()";
		pin->Release();
		pin = NULL;
	}
	LOG(INFO) << " GetOutputPin  pPinEnum->Release()";
	pPinEnum->Release();
	return NULL;
}

BOOL PinMatchesCategory(IPin *pPin, REFGUID Category)
{
	BOOL bFound = FALSE;
	IKsPropertySet *pKs = NULL;
	HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pKs));
	if (SUCCEEDED(hr)) {
		GUID PinCategory;
		DWORD cbReturned;
		hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &PinCategory,
			sizeof(GUID), &cbReturned);
		if (SUCCEEDED(hr) && (cbReturned == sizeof(GUID))) {
			bFound = (PinCategory == Category);
		}
		pKs->Release();
	}
	return bFound;
}