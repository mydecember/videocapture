#pragma once
#define NO_DSHOW_STRSAFE
#include <dshow.h>
#include "memory"
#include "Comm.h"
class FilterBase
	: public IBaseFilter, public RefCount<FilterBase> {
public:
	FilterBase();
	virtual ~FilterBase();

	// Number of pins connected to this filter.
	virtual size_t NoOfPins() = 0;
	// Returns the IPin interface pin no index.
	virtual IPin* GetPin(int index) = 0;

	// Inherited from IUnknown.
	STDMETHOD(QueryInterface)(REFIID id, void** object_ptr);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// Inherited from IBaseFilter.
	STDMETHOD(EnumPins)(IEnumPins** enum_pins);

	STDMETHOD(FindPin)(LPCWSTR id, IPin** pin);

	STDMETHOD(QueryFilterInfo)(FILTER_INFO* info);

	STDMETHOD(JoinFilterGraph)(IFilterGraph* graph, LPCWSTR name);

	STDMETHOD(QueryVendorInfo)(LPWSTR* vendor_info);

	// Inherited from IMediaFilter.
	STDMETHOD(Stop)();

	STDMETHOD(Pause)();

	STDMETHOD(Run)(REFERENCE_TIME start);

	STDMETHOD(GetState)(DWORD msec_timeout, FILTER_STATE* state);

	STDMETHOD(SetSyncSource)(IReferenceClock* clock);

	STDMETHOD(GetSyncSource)(IReferenceClock** clock);

	// Inherited from IPersistent.
	STDMETHOD(GetClassID)(CLSID* class_id) = 0;

private:
	FILTER_STATE state_;
	IFilterGraph* owning_graph_;

};

