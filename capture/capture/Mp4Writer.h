#pragma once
#include "Comm.h"
class Mp4Writer
{
public:
	Mp4Writer();
	~Mp4Writer();
	AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, VideoCaptureCapability frameInfo);
	void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	int CreateMp4(const char* filename, VideoCaptureCapability frameInfo);
	void WriteVideo(void* data, int nLen);
	void CloseMp4();
	void WriteRawdata(const char*buf, int len);
private:
	bool isIdrFrame1(uint8_t* buf, int size);
	bool isIdrFrame2(uint8_t* buf, int len);
private:
	std::string file_name_;
	int ptsInc_ = 0;
	int waitkey_ = 1;;
	const int STREAM_FRAME_RATE = 25;
	AVFormatContext* format_context_;
	int vi_;
	enum AVPixelFormat src_pix_fmt_;
	AVCodecContext *pcodeCtx_;
	int width_;
	int height_;
	int fps_;
};

