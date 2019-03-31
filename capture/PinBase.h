#pragma once
#define NO_DSHOW_STRSAFE
#include <dshow.h>
#include "memory"
#include"wrl/client.h"
#include "Comm.h"
//#include "streams.h"
class PinBase
	: public IPin,
	public IMemInputPin, public RefCount<PinBase> {
public:
	explicit PinBase(IBaseFilter* owner);
	virtual ~PinBase();

	// Function used for changing the owner.
	// If the owner is deleted the owner should first call this function
	// with owner = NULL.
	void SetOwner(IBaseFilter* owner);
	IBaseFilter* GetOwner();

	// Checks if a media type is acceptable. This is called when this pin is
	// connected to an output pin. Must return true if the media type is
	// acceptable, false otherwise.
	virtual bool IsMediaTypeValid(const AM_MEDIA_TYPE* media_type) = 0;

	// Enumerates valid media types.
	virtual bool GetValidMediaType(int index, AM_MEDIA_TYPE* media_type) = 0;

	// Called when new media is received. Note that this is not on the same
	// thread as where the pin is created.
	STDMETHOD(Receive)(IMediaSample* sample) = 0;

	STDMETHOD(Connect)(IPin* receive_pin, const AM_MEDIA_TYPE* media_type);

	STDMETHOD(ReceiveConnection)(IPin* connector,
		const AM_MEDIA_TYPE* media_type);

	STDMETHOD(Disconnect)();

	STDMETHOD(ConnectedTo)(IPin** pin);

	STDMETHOD(ConnectionMediaType)(AM_MEDIA_TYPE* media_type);

	STDMETHOD(QueryPinInfo)(PIN_INFO* info);

	STDMETHOD(QueryDirection)(PIN_DIRECTION* pin_dir);

	STDMETHOD(QueryId)(LPWSTR* id);

	STDMETHOD(QueryAccept)(const AM_MEDIA_TYPE* media_type);

	STDMETHOD(EnumMediaTypes)(IEnumMediaTypes** types);

	STDMETHOD(QueryInternalConnections)(IPin** pins, ULONG* no_pins);

	STDMETHOD(EndOfStream)();

	STDMETHOD(BeginFlush)();

	STDMETHOD(EndFlush)();

	STDMETHOD(NewSegment)(REFERENCE_TIME start,
		REFERENCE_TIME stop,
		double dRate);

	// Inherited from IMemInputPin.
	STDMETHOD(GetAllocator)(IMemAllocator** allocator);

	STDMETHOD(NotifyAllocator)(IMemAllocator* allocator, BOOL read_only);

	STDMETHOD(GetAllocatorRequirements)(ALLOCATOR_PROPERTIES* properties);

	STDMETHOD(ReceiveMultiple)(IMediaSample** samples,
		long sample_count,
		long* processed);
	STDMETHOD(ReceiveCanBlock)();

	// Inherited from IUnknown.
	STDMETHOD(QueryInterface)(REFIID id, void**object_ptr);

	STDMETHOD_(ULONG, AddRef)();

	STDMETHOD_(ULONG, Release)();

private:
	AM_MEDIA_TYPE current_media_type_;
	Microsoft::WRL::ComPtr<IPin> connected_pin_;
	//IPin* connected_pin_;
	// owner_ is the filter owning this pin. We don't reference count it since
	// that would create a circular reference count.
	IBaseFilter* owner_;
};

