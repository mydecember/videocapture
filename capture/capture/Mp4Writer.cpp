#include "stdafx.h"
#include "Mp4Writer.h"


Mp4Writer::Mp4Writer()
{
	ptsInc_ = 0;
	waitkey_ = 1;;
}


Mp4Writer::~Mp4Writer()
{
}

bool Mp4Writer::isIdrFrame2(uint8_t* buf, int len) {

	switch (buf[0] & 0x1f) {
	case 7: // SPS
		return true;
	case 8: // PPS
		return true;
	case 5:
		return true;
	case 1:
		return false;

	default:
		return false;
		break;
	}

	return false;
}
bool Mp4Writer::isIdrFrame1(uint8_t* buf, int size) {
	//主要是解析idr前面的sps pps
	//    static bool found = false;
	//    if(found){ return true;}

	int last = 0;
	for (int i = 2; i <= size; ++i) {
		if (i == size) {
			if (last) {
				bool ret = isIdrFrame2(buf + last, i - last);
				if (ret) {
					//found = true;
					return true;
				}
			}
		}
		else if (buf[i - 2] == 0x00 && buf[i - 1] == 0x00 && buf[i] == 0x01) {
			if (last) {
				int size = i - last - 3;
				if (buf[i - 3]) ++size;
				bool ret = isIdrFrame2(buf + last, size);
				if (ret) {
					//found = true;
					return true;
				}
			}
			last = i + 1;
		}
	}
	return false;

}

/* Add an output stream */
AVStream *Mp4Writer::add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, VideoCaptureCapability frameInfo)
{
	AVCodecContext *c;
	AVStream *st;
	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!*codec)
	{
		printf("could not find encoder for '%s' \n", avcodec_get_name(codec_id));
		exit(1);
	}
	st = avformat_new_stream(oc, *codec);
	if (!st)
	{
		printf("could not allocate stream \n");
		exit(1);
	}
	st->id = oc->nb_streams - 1;
	c = st->codec;
	vi_ = st->index;
	switch ((*codec)->type)
	{
	case AVMEDIA_TYPE_AUDIO:
		printf("AVMEDIA_TYPE_AUDIO\n");
		c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 64000;
		c->sample_rate = 44100;
		c->channels = 2;
		break;
	case AVMEDIA_TYPE_VIDEO:
		printf("AVMEDIA_TYPE_VIDEO\n");
		pcodeCtx_ = c;
		c->codec_id = AV_CODEC_ID_H264;
		c->bit_rate = 0;
		c->width = frameInfo.width;//1080;
		c->height = frameInfo.height;//720;
		
		c->time_base.den = 50;
		c->time_base.num = 1;
		c->gop_size = 10;
		c->pix_fmt = AV_PIX_FMT_YUV420P;
		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		{
			c->max_b_frames = 2;
		}
		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
		{
			c->mb_decision = 2;
		}
		break;
	default:
		break;
	}
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	{
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	return st;
}
void Mp4Writer::open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
	int ret;
	AVCodecContext *c = st->codec;
	/* open the codec */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0)
	{
		printf("could not open video codec");
		//exit(1);
	}
}
int Mp4Writer::CreateMp4(const char* filename, VideoCaptureCapability frameInfo)
{
	int ret; // 成功返回0，失败返回1
	const char* pszFileName = filename;
	AVOutputFormat *fmt;
	AVCodec *video_codec;
	AVStream *m_pVideoSt;
	av_register_all();
	src_pix_fmt_ = RawTypeToAVFormat(frameInfo.rawType);
	width_ = frameInfo.width;
	height_ = frameInfo.height;
	fps_ = frameInfo.maxFPS;

	avformat_alloc_output_context2(&format_context_, NULL, NULL, pszFileName);
	if (!format_context_)
	{
		printf("Could not deduce output format from file extension: using MPEG. \n");
		avformat_alloc_output_context2(&format_context_, NULL, "mpeg", pszFileName);
	}
	if (!format_context_)
	{
		return 1;
	}
	fmt = format_context_->oformat;
	if (fmt->video_codec != AV_CODEC_ID_NONE)
	{
		printf("1111111111111111add_stream\n");
		m_pVideoSt = add_stream(format_context_, &video_codec, fmt->video_codec, frameInfo);
	}
	if (m_pVideoSt)
	{
		printf("1111111111111111open_video\n");
		open_video(format_context_, video_codec, m_pVideoSt);
	}
	printf("==========Output Information==========\n");
	av_dump_format(format_context_, 0, pszFileName, 1);
	printf("======================================\n");
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&format_context_->pb, pszFileName, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("could not open %s\n", pszFileName);
			return 1;
		}
	}
	/* Write the stream header, if any */
	ret = avformat_write_header(format_context_, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening output file");
		return 1;
	}
}
/* write h264 data to mp4 file

* 创建mp4文件返回2；写入数据帧返回0 */
void Mp4Writer::WriteVideo(void* data, int nLen)
{
	int ret;
	if (0 > vi_)
	{
		printf("vi less than 0\n");
		//return -1;
	}
	AVStream *pst = format_context_->streams[vi_];
	//printf("vi=====%d\n",vi);
	// Init packet
	AVPacket pkt;
	// 我的添加，为了计算pts
	AVCodecContext *c = pst->codec;
	av_init_packet(&pkt);
	int isI = isIdrFrame1((uint8_t*)data, nLen);
	printf("isIFrame is %d\n", isI);
	pkt.flags |= isI ? AV_PKT_FLAG_KEY : 0;
	pkt.stream_index = pst->index;
	pkt.data = (uint8_t*)data;
	pkt.size = nLen;
	// Wait for key frame
	if (waitkey_) {
		if (0 == (pkt.flags & AV_PKT_FLAG_KEY)) {
			return;
		}
		else
			waitkey_ = 0;
	}
	pkt.pts = (ptsInc_++) * (90000 / STREAM_FRAME_RATE);
	pkt.pts = av_rescale_q((ptsInc_++) * 2, pst->codec->time_base, pst->time_base);
	//pkt.dts = (ptsInc++) * (90000/STREAM_FRAME_RATE);
	//  pkt.pts=av_rescale_q_rnd(pkt.pts, pst->time_base,pst->time_base,(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
	pkt.dts = av_rescale_q_rnd(pkt.dts, pst->time_base, pst->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	pkt.duration = av_rescale_q(pkt.duration, pst->time_base, pst->time_base);
	pkt.pos = -1;
	printf("pkt.size=%d\n", pkt.size);
	ret = av_interleaved_write_frame(format_context_, &pkt);
	if (ret < 0)
	{
		printf("cannot write frame");
	}
}
void Mp4Writer::CloseMp4()
{
	waitkey_ = -1;
	vi_ = -1;
	if (format_context_)
		av_write_trailer(format_context_);
	if (format_context_ && !(format_context_->oformat->flags & AVFMT_NOFILE))
		avio_close(format_context_->pb);
	if (format_context_)
	{
		avformat_free_context(format_context_);
		format_context_ = NULL;
	}
}

void Mp4Writer::WriteRawdata(const char*buf, int len) {
	enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;

	//uint8_t* rgbbuf = (uint8_t*)malloc(frameInfo.width * frameInfo.height * 3);
	AVFrame *pFrameYUV = av_frame_alloc();
	AVFrame *toFrame = av_frame_alloc();
	uint8_t *out_buffer = new uint8_t[avpicture_get_size(dst_pix_fmt, width_, height_)];
	avpicture_fill((AVPicture *)pFrameYUV, (const uint8_t*)buf, src_pix_fmt_, width_, height_);

	avpicture_fill((AVPicture *)toFrame, out_buffer, dst_pix_fmt, width_, height_);



	SwsContext *sws_ctx = sws_getContext(
		width_, height_, src_pix_fmt_,
		width_, height_, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);

	//pFrameYUV->data[0] = pFrameYUV->data[0] + pFrameYUV->linesize[0] * (frameInfo.height - 1);
	//pFrameYUV->linesize[0] *= -1;
	//pFrameYUV->data[1] = pFrameYUV->data[1] + pFrameYUV->linesize[1] * (frameInfo.height / 2 - 1);
	//pFrameYUV->linesize[1] *= -1;
	//pFrameYUV->data[2] = pFrameYUV->data[2] + pFrameYUV->linesize[2] * (frameInfo.height / 2 - 1);
	//pFrameYUV->linesize[2] *= -1;

	sws_scale(sws_ctx, pFrameYUV->data, pFrameYUV->linesize, 0, height_, toFrame->data, toFrame->linesize);
	sws_freeContext(sws_ctx);


	int ret, got_frame = 0;
	AVPacket pkt;

	ret = avcodec_encode_audio2(pcodeCtx_, &pkt, toFrame, &got_frame);
	if (ret < 0) {
		printf("error encoder\n");
	}
	if (got_frame == 1) {
		
			//WriteVideo(toFrame->data[0], avpicture_get_size(dst_pix_fmt, width_, height_));
	}

	WriteVideo(toFrame->data[0], avpicture_get_size(dst_pix_fmt, width_, height_));
}