#include "stdafx.h"
#include "Mp4Writer.h"


Mp4Writer::Mp4Writer()
{
	started_ = false;
	ptsInc_ = 0;
	waitkey_ = 1;
	format_context_ = NULL;
	vi_ = -1;

	pcodeCtx_ = NULL;
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
		LOG(INFO) << "could not find encoder for '%s' " << avcodec_get_name(codec_id);
		exit(1);
	}
	st = avformat_new_stream(oc, *codec);
	if (!st)
	{
		LOG(INFO) << ("could not allocate stream \n");
		exit(1);
	}
	st->id = oc->nb_streams - 1;

	//pcodeCtx_ = avcodec_alloc_context3(*codec);
	c = st->codec;
	vi_ = st->index;
	switch ((*codec)->type)
	{
	case AVMEDIA_TYPE_AUDIO:
		LOG(INFO) << ("AVMEDIA_TYPE_AUDIO\n");
		c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 64000;
		c->sample_rate = 44100;
		c->channels = 2;
		break;
	case AVMEDIA_TYPE_VIDEO:
		LOG(INFO) << ("AVMEDIA_TYPE_VIDEO\n");
		pcodeCtx_ = c;
		c->codec_id = AV_CODEC_ID_H264;
		c->bit_rate = 400000;// 10000000;
		c->width = frameInfo.width;//1080;
		c->height = frameInfo.height;//720;
		
		c->time_base.den = frameInfo.maxFPS;
		c->time_base.num = 1;
		st->time_base = c->time_base;

		c->pix_fmt = AV_PIX_FMT_YUV420P;

		c->gop_size = 10;
		c->pix_fmt = AV_PIX_FMT_YUV420P;
		c->max_b_frames = 1;
		c->profile = FF_PROFILE_HEVC_MAIN;
		c->level = 0x28;
		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
		{
			c->max_b_frames = 2;
		}
		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
		{
			c->mb_decision = 2;
		}
		if ((*codec)->id == AV_CODEC_ID_H264) {
			av_opt_set(c->priv_data, "preset", "slow", 0);
		}
		
		break;
	default:
		break;
	}
	/*if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	{
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}*/
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
		LOG(INFO) << ("could not open video codec");
		//exit(1);
	}
}
int Mp4Writer::CreateMp4(std::string filename, VideoCaptureCapability frameInfo)
{
	file_name_ = filename;
	int ret; // 成功返回0，失败返回1
	const char* pszFileName = filename.c_str();

	AVOutputFormat *fmt;
	AVCodec *video_codec = NULL;
	AVStream *m_pVideoSt = NULL;
	av_register_all();

	src_pix_fmt_ = RawTypeToAVFormat(frameInfo.rawType);
	width_ = frameInfo.width;
	height_ = frameInfo.height;
	fps_ = frameInfo.maxFPS;

	avformat_alloc_output_context2(&format_context_, NULL, NULL, pszFileName);
	if (!format_context_)
	{
		LOG(INFO) << ("Could not deduce output format from file extension: using MPEG. \n");
		avformat_alloc_output_context2(&format_context_, NULL, "mpeg", pszFileName);
	}
	if (!format_context_)
	{
		return 1;
	}
	fmt = format_context_->oformat;
	LOG(INFO) << " cccccccccccc type " << fmt->video_codec <<" pix fmt " << src_pix_fmt_;
	if (fmt->video_codec != AV_CODEC_ID_NONE)
	{
		LOG(INFO) << "1111111111111111add_stream codec type " << fmt->video_codec;
		m_pVideoSt = add_stream(format_context_, &video_codec, fmt->video_codec, frameInfo);
	}
	if (m_pVideoSt)
	{
		LOG(INFO) << "1111111111111111open_video\n";
		open_video(format_context_, video_codec, m_pVideoSt);
	}
	LOG(INFO) << ("==========Output Information==========\n");
	//av_dump_format(format_context_, 0, pszFileName, 1);
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE))
	{
		LOG(INFO) << "to open avio " << pszFileName;
		ret = avio_open(&format_context_->pb, pszFileName, AVIO_FLAG_READ_WRITE);
		if (ret < 0)
		{
			LOG(INFO) << "could not open " << pszFileName;
			return 1;
		}
	}
	else {
		LOG(INFO) << "AVFMT_NOFILE ERROR";
	}
	/* Write the stream header, if any */
	ret = avformat_write_header(format_context_, NULL);
	if (ret < 0)
	{
		LOG(INFO) << ("Error occurred when opening output file");
		return 1;
	}
	LOG(INFO) << "create mp4 file " << file_name_  << " capabilities " << video_codec->capabilities;
	started_ = true;

	init_decoder();


	output_.open("./sample.h264", std::ofstream::binary);
}


void Mp4Writer::CloseMp4()
{
	output_.close();
	started_ = false;
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

void Mp4Writer::init_decoder() {
	


	/* find the MPEG-1 video decoder */
	dec_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!dec_codec) {
		LOG(INFO) << " " << "Codec not found\n";
		return;
	}

	dec_context = avcodec_alloc_context3(dec_codec);
	if (!dec_context) {
		LOG(INFO) << "Could not allocate video codec context\n";
		exit(1);
	}

	if (dec_codec->capabilities&CODEC_CAP_TRUNCATED)
		dec_context->flags |= CODEC_FLAG_TRUNCATED;
	/* open it */
	if (avcodec_open2(dec_context, dec_codec, NULL) < 0) {
		LOG(INFO) << "Could not open codec\n";
		exit(1);
	}
	LOG(INFO) << "============ decoder init ok\n";

}
void Mp4Writer::video_decode_example(AVPacket& pkt)
{
	AVFrame *frame;
	frame = av_frame_alloc();
	if (!frame) {
		LOG(INFO) << "Could not allocate video frame\n";
		exit(1);
	}

	static int frame_count = 0;

	int len, got_frame;
	char buf[1024];
	dec_context->time_base;

	len = avcodec_decode_video2(dec_context, frame, &got_frame, &pkt);
	if (len < 0) {
		LOG(INFO) << "Error while decoding frame" << frame_count;
		return ;
	}
	if (got_frame) {
		LOG(INFO) << " decode type " << frame->pict_type
			<< " pts: " << frame->pts
			<< " pkt_pts:" << frame->pkt_pts
			<< " pkt_dts: " << frame->pkt_dts;



		
		frame_count++;
		av_frame_free(&frame);
	}
	else {
		LOG(INFO) << " decode none ";
	}

	return ;



}


uint8_t* FindSep(uint8_t *pdata, int size, int &flag)
{
	uint8_t *p = pdata;
	for (int i = 0; i< size; ++i)
	{
		if (pdata[i] == 0 && pdata[i + 1] == 0 && (pdata[i + 2] == 1 || (pdata[i + 2] == 0 && pdata[i + 3] == 1)))
		{

			//i += (pdata[i+2] == 1) ? 3 : 4;
			if (pdata[i + 2] == 1)
			{
				flag = 3;
				return p + i + 3;
			}
			else
			{
				flag = 4;
				return p + i + 4;
			}

		}
	}
	return NULL;
}

void Mp4Writer::WriteRawdata(char*buf, int len) {
	static int i = 0;
	i++;
	enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;

	//uint8_t* rgbbuf = (uint8_t*)malloc(frameInfo.width * frameInfo.height * 3);
	AVFrame *pFrameYUV = av_frame_alloc();
	AVFrame *toFrame = av_frame_alloc();
	uint8_t *out_buffer = (uint8_t*)malloc(avpicture_get_size(dst_pix_fmt, width_, height_));
	avpicture_fill((AVPicture *)pFrameYUV, (const uint8_t*)buf, src_pix_fmt_, width_, height_);

	avpicture_fill((AVPicture *)toFrame, out_buffer, dst_pix_fmt, width_, height_);



	SwsContext *sws_ctx = sws_getContext(
		width_, height_, src_pix_fmt_,
		width_, height_, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);


	sws_scale(sws_ctx, pFrameYUV->data, pFrameYUV->linesize, 0, height_, toFrame->data, toFrame->linesize);
	sws_freeContext(sws_ctx);


	int ret, got_frame = 0;
	AVPacket pkt;
	av_init_packet(&pkt);

	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;

	toFrame->pts = i;
	pcodeCtx_->extradata;//////////// TODO find 
	ret = avcodec_encode_video2(pcodeCtx_, &pkt, toFrame, &got_frame);
	if (ret < 0) {
		LOG(INFO) <<"error encoder " <<ret ;
	}
	if (got_frame == 1) {
		output_.write((char*)pkt.data, pkt.size);
		uint8_t *side_data;
		int side_data_size;
		int i;
		pkt.data;

		//side_data = av_packet_get_side_data(*pkt, AV_PKT_DATA_QUALITY_STATS, &side_data_size);

		LOG(INFO) << "encoded one frame " << pkt.size << " context num:" << pcodeCtx_->time_base.num << " den:" <<pcodeCtx_->time_base.den
			<< " pst num:" << format_context_->streams[vi_]->time_base.num <<" den:" << format_context_->streams[vi_]->time_base.den
			<<" pts :" << pkt.pts <<" dts:" << pkt.dts << "  Frame pts " << toFrame->pts << " pkd_dts " << toFrame->pkt_dts
			<<" type " << pcodeCtx_->coded_frame->pict_type ;

		int sepNum;
		int size = pkt.size;
		uint8_t* index = FindSep(pkt.data, pkt.size, sepNum);
		if (index == NULL) {
			LOG(INFO) << "not find  annex";
		}
		size -= sepNum;
		while (size >0 && index) {
			uint8_t* now = FindSep(index, pkt.size, sepNum);

			LOG(INFO) << " nal type " << (*index & 0x1F);
			size -= (index - now);
			index = now ;
		}


		video_decode_example(pkt);
			//WriteVideo(pkt.data, pkt.size);
		AVStream *pst = format_context_->streams[vi_];
		av_packet_rescale_ts(&pkt, pcodeCtx_->time_base, pst->time_base);
		av_interleaved_write_frame(format_context_, &pkt);
		av_free_packet(&pkt);
	}
	else {
		LOG(INFO) << "no response " << "  Frame pts " << toFrame->pts;
	}
	free(out_buffer);

	//WriteVideo(toFrame->data[0], avpicture_get_size(dst_pix_fmt, width_, height_));
}