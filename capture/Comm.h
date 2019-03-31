#pragma once
#include "stdint.h"
#include "logging.h"

#define __STDC_CONSTANT_MACROS     
//Windows    
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"    
#include "libavformat/avformat.h"    
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
};
// Video codec types

enum VideoCodecType {
	kVideoCodecH264,
	kVideoCodecH264SVC,
	kVideoCodecI420,
	kVideoCodecRED,
	kVideoCodecULPFEC,
	kVideoCodecGeneric,
	kVideoCodecUnknown
};
enum RawVideoType {
	kVideoI420 = 0,
	kVideoYV12 = 1,
	kVideoYUY2 = 2,
	kVideoUYVY = 3,
	kVideoIYUV = 4,
	kVideoARGB = 5,
	kVideoRGB24 = 6,
	kVideoRGB565 = 7,
	kVideoARGB4444 = 8,
	kVideoARGB1555 = 9,
	kVideoMJPEG = 10,
	kVideoNV12 = 11,
	kVideoNV21 = 12,
	kVideoBGRA = 13,
	kVideoUnknown = 99
};

AVPixelFormat RawTypeToAVFormat(RawVideoType type);
enum { kVideoCaptureUniqueNameLength = 1024 }; //Max unique capture device name lenght
enum { kVideoCaptureDeviceNameLength = 256 }; //Max capture device name lenght
enum { kVideoCaptureProductIdLength = 128 }; //Max product id length

											 // Enums
enum VideoCaptureRotation {
	kCameraRotate0 = 0,
	kCameraRotate90 = 5,
	kCameraRotate180 = 10,
	kCameraRotate270 = 15
};

struct VideoCaptureCapability {
	int32_t width;
	int32_t height;
	int32_t maxFPS;
	int32_t expectedCaptureDelay;
	RawVideoType rawType;
	VideoCodecType codecType;
	bool interlaced;

	VideoCaptureCapability() {
		width = 0;
		height = 0;
		maxFPS = 0;
		expectedCaptureDelay = 0;
		rawType = kVideoUnknown;
		codecType = kVideoCodecUnknown;
		interlaced = false;
	}
	;
	bool operator!=(const VideoCaptureCapability &other) const {
		if (width != other.width)
			return true;
		if (height != other.height)
			return true;
		if (maxFPS != other.maxFPS)
			return true;
		if (rawType != other.rawType)
			return true;
		if (codecType != other.codecType)
			return true;
		if (interlaced != other.interlaced)
			return true;
		return false;
	}
	bool operator==(const VideoCaptureCapability &other) const {
		return !operator!=(other);
	}
};

enum VideoCaptureAlarm {
	Raised = 0,
	Cleared = 1
};

/* External Capture interface. Returned by Create
and implemented by the capture module.
*/
class IExternalSourcedVideoCapture
{
public:
	// |capture_time| must be specified in the NTP time format in milliseconds.
	virtual int32_t IncomingFrame(uint8_t* videoFrame,
		int32_t videoFrameLength,
		const VideoCaptureCapability& frameInfo,
		int64_t captureTime = 0,
		int rotationAngle = 0) = 0;
	/*virtual int32_t IncomingI420VideoFrame(I420VideoFrame* video_frame,
	int64_t captureTime = 0) = 0;*/

protected:
	~IExternalSourcedVideoCapture() {}
};

// Callback class to be implemented by module user
class VideoCaptureDataCallback
{
public:
	//virtual void OnIncomingCapturedFrame(const int32_t id,
	//	I420VideoFrame& videoFrame) = 0;
	virtual void OnCaptureDelayChanged(const int32_t id,
		const int32_t delay) = 0;
protected:
	virtual ~VideoCaptureDataCallback() {}
};

class VideoCaptureFeedBack
{
public:
	virtual void OnCaptureFrameRate(const int32_t id,
		const uint32_t frameRate) = 0;
	virtual void OnNoPictureAlarm(const int32_t id,
		const VideoCaptureAlarm alarm) = 0;
protected:
	virtual ~VideoCaptureFeedBack() {}
};

struct VideoCaptureCapabilityWinDS : public VideoCaptureCapability {
	uint32_t directShowCapabilityIndex;
	bool supportFrameRateControl;
	VideoCaptureCapabilityWinDS() {
		directShowCapabilityIndex = 0;
		supportFrameRateControl = false;
	}
};
class Comm
{
public:
	Comm();
	~Comm();
};

template <typename T>
class RefCount {
public:
	RefCount():ref_count_(0){}
	void AddRef() {
		++ref_count_;
	}
	int Release() {
		 --ref_count_;
		 if (ref_count_ == 0) {
			 //delete static_cast<const T*>(this);
			 LOG(INFO) <<" to delte this " <<this;
		 }
		 return 1;
	}
protected:
	~RefCount() {}
private:
	RefCount<T>(const RefCount<T>&);
	void operator =(const RefCount<T>&);
	int ref_count_;
};

template <class T>
class scoped_refptr
{
public:
	scoped_refptr() : ptr_(NULL) {
	}

	scoped_refptr(T* p) : ptr_(p) {
		if (ptr_)
			ptr_->AddRef();
	}

	scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
		if (ptr_)
			ptr_->AddRef();
	}

	template <typename U>
	scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
		if (ptr_)
			ptr_->AddRef();
	}

	~scoped_refptr() {
		if (ptr_)
			ptr_->Release();
	}

	T* get() const {
		return ptr_;
	}
	operator T*() const {
		return ptr_;
	}
	T* operator->() const {
		return ptr_;
	}

	void reset(T* p) {
		// This is a self-reset, which is no longer allowed: http://crbug.com/162971
		if (p != NULL && p == ptr_)
			return;
		*this = p;
	}

	void attach(T* p) {
		ASSERT(ptr_ == NULL);
		ptr_ = p;
	}

	// Release a pointer.
	// The return value is the current pointer held by this object.
	// If this object holds a NULL pointer, the return value is NULL.
	// After this operation, this object will hold a NULL pointer,
	// and will not own the object any more.
	T* release() {
		T* retVal = ptr_;
		ptr_ = NULL;
		return retVal;
	}

	scoped_refptr<T>& operator=(T* p) {
		// AddRef first so that self assignment should work
		if (p)
			p->AddRef();
		if (ptr_)
			ptr_->Release();
		ptr_ = p;
		return *this;
	}

	scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
		return *this = r.ptr_;
	}

	template <typename U>
	scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
		return *this = r.get();
	}

	void swap(T** pp) {
		T* p = ptr_;
		ptr_ = *pp;
		*pp = p;
	}

	void swap(scoped_refptr<T>& r) {
		swap(&r.ptr_);
	}

protected:
	T * ptr_;
};


// Handy utility for creating a scoped_refptr<T> out of a T* explicitly without
// having to retype all the template arguments
template <typename T>
scoped_refptr<T> make_scoped_refptr(T* t) {
	return scoped_refptr<T>(t);
}

template <typename T, typename U>
bool operator==(const scoped_refptr<T>& lhs, const U* rhs) {
	return lhs.get() == rhs;
}

template <typename T, typename U>
bool operator==(const T* lhs, const scoped_refptr<U>& rhs) {
	return lhs == rhs.get();
}

template <typename T>
bool operator==(const scoped_refptr<T>& lhs, std::nullptr_t null) {
	return !static_cast<bool>(lhs);
}

template <typename T>
bool operator==(std::nullptr_t null, const scoped_refptr<T>& rhs) {
	return !static_cast<bool>(rhs);
}

template <typename T, typename U>
bool operator!=(const scoped_refptr<T>& lhs, const U* rhs) {
	return !operator==(lhs, rhs);
}

template <typename T, typename U>
bool operator!=(const T* lhs, const scoped_refptr<U>& rhs) {
	return !operator==(lhs, rhs);
}

template <typename T>
bool operator!=(const scoped_refptr<T>& lhs, std::nullptr_t null) {
	return !operator==(lhs, null);
}

template <typename T>
bool operator!=(std::nullptr_t null, const scoped_refptr<T>& rhs) {
	return !operator==(null, rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const scoped_refptr<T>& p) {
	return out << p.get();
}

class VideoCaptureObserver {
public:
	virtual void OnReceiveFrame(unsigned char* pBuffer, int32_t length, VideoCaptureCapability frameInfo) {};
};