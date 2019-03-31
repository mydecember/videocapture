#pragma once
#include "Comm.h"
#include <fstream>
#include <fstream>
#include <sstream>

class Mp4Writer
{
public:
	Mp4Writer();
	~Mp4Writer();
	

	int CreateMp4(std::string filename, VideoCaptureCapability frameInfo);
	void WriteVideo(void* data, int nLen);
	void CloseMp4();
	void WriteRawdata(char*buf, int len);
private:
	void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
	AVStream * add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, VideoCaptureCapability frameInfo);
	bool isIdrFrame1(uint8_t* buf, int size);
	bool isIdrFrame2(uint8_t* buf, int len);

	void init_decoder();
	void video_decode_example(AVPacket& pkt);

private:
	bool started_;
	std::string file_name_;
	int ptsInc_ = 0;
	int waitkey_ = 1;;
	const int STREAM_FRAME_RATE = 25;
	AVFormatContext* format_context_;
	int vi_;
	AVPixelFormat src_pix_fmt_;
	AVCodecContext *pcodeCtx_;
	int width_;
	int height_;
	int fps_;

	AVFrame *frame_;
	AVPacket pkt_;
	const AVCodec *codec_;

	AVCodec *dec_codec;
	AVCodecContext *dec_context = NULL;
	std::ofstream output_;
};

