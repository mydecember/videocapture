#include "stdafx.h"
#include "HDCRender.h"
#include "Comm.h"
#pragma comment(lib, "Vfw32.lib")
HDCRender::HDCRender(HWND newhand)
{
	haveInit = 0;
	m_wndChange = 0;
	//m_winRect={0,0,0,0};
	//m_clientRect={0,0,0,0};
	memset(&m_winRect, 0, sizeof(m_winRect));
	memset(&m_clientRect, 0, sizeof(m_clientRect));
	/*if(stc_objId >= 100000)
	stc_objId = 0;
	objID = stc_objId;
	stc_objId++;*/

	m_wndWidth = 640;
	m_wndHeight = 360;

	m_IMAGE_WIDTH = 640;
	m_IMAGE_HEIGHT = 360;

	hdib = DrawDibOpen();
	LOG(INFO) << "newhand =" << newhand;
	if (newhand != 0)
	{
		handl = newhand;
		if ((newhand != (HWND)-1) && (newhand != (HWND)0))
			m_hdc = ::GetDC((HWND)newhand);//this->GetDC()->m_hDC;
	}
	m_stat = 0;
}


HDCRender::~HDCRender()
{
	DrawDibClose(hdib);
	haveInit = 0;
	m_wndChange = 0;
	memset(&m_winRect, 0, sizeof(m_winRect));
	memset(&m_clientRect, 0, sizeof(m_clientRect));
	LOG(INFO) << "Destory ================ video Wnd" << std::endl;

	ReleaseDC(handl, m_hdc);
}

void HDCRender::InitDCParam()
{

	if (m_wndChange)
	{
		if (handlNew == (HWND)-1)
		{
			DrawDibClose(hdib);
			ReleaseDC(handl, m_hdc);
			handl = handlNew;
			hdib = DrawDibOpen();
			m_wndChange = 0;
			handlNew = 0;
		}
		else
		{
			DrawDibClose(hdib);
			ReleaseDC(handl, m_hdc);
			m_hdc = ::GetDC((HWND)handlNew);//this->GetDC()->m_hDC;
			handl = handlNew;
			hdib = DrawDibOpen();

		}


	}
	if (handl == (HWND)-1)
		return;

	// Get Dialog DC

	RECT rect1, rect2;
	::GetWindowRect(handl, &rect1);
	GetClientRect(handl, &rect2);

	m_clientRect = rect2;
	m_winRect = rect1;
	m_wndWidth = rect2.right;
	m_wndHeight = rect2.bottom;


	if (m_clientRect.bottom != rect2.bottom || m_clientRect.left != rect2.left
		|| m_clientRect.right != rect2.right || m_clientRect.top != rect2.top || m_wndChange || !haveInit)
	{
		m_bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		m_bmpinfo.bmiHeader.biWidth = m_IMAGE_WIDTH;//IMAGE_WIDTH
		m_bmpinfo.bmiHeader.biHeight = m_IMAGE_HEIGHT;//IMAGE_HEIGHT
		m_bmpinfo.bmiHeader.biPlanes = 1;
		m_bmpinfo.bmiHeader.biBitCount = 24;
		m_bmpinfo.bmiHeader.biCompression = 0;
		m_bmpinfo.bmiHeader.biSizeImage = m_IMAGE_WIDTH * m_IMAGE_HEIGHT * 3;//IMAGE_HEIGHT*IMAGE_WIDTH*2
		m_bmpinfo.bmiHeader.biXPelsPerMeter = 0;
		m_bmpinfo.bmiHeader.biYPelsPerMeter = 0;
		m_bmpinfo.bmiHeader.biClrUsed = 0;
		m_bmpinfo.bmiHeader.biClrImportant = 0;

		if (hdib != NULL)
		{

			DrawDibBegin(hdib,
				m_hdc,
				-1,				// don't stretch, -1
				-1,				// don't stretch, -1
				&m_bmpinfo.bmiHeader,
				m_IMAGE_WIDTH,         // width of image, IMAGE_WIDTH
				m_IMAGE_HEIGHT,        // height of image, IMAGE_HEIGHT
				0
			);
		}
		m_wndChange = 0;
		haveInit = 1;
	}//end if

}

void HDCRender::PrintVideoData(BYTE *data, int nWidth, int nHeight)
{
	m_IMAGE_WIDTH = nWidth;
	m_IMAGE_HEIGHT = nHeight;

	InitDCParam();
	if (m_wndChange) {
		LOG(INFO) << " wnd change ========";
		return;
	}

	if (handl == (HWND)-1 || handl == (HWND)0) {
		LOG(INFO) << " wnd is null";
		return;
	}

	int x = m_clientRect.left;
	int y = m_clientRect.top;
	int xc = m_wndWidth;
	int yc = m_wndHeight;
	DrawDibDraw(hdib,
		m_hdc,
		x,		// dest : left pos x
		y,		// dest : top pos y
		xc,					 // don't zoom x xc
		yc,					 // don't zoom y yc
		&m_bmpinfo.bmiHeader,			 // bmp header info
		data,					 // bmp data
		0,					 // src :left
		0,					 // src :top
		nWidth,				 // src : width,  //IMAGE_WIDTH nWidth
		nHeight,				 // src : height	//IMAGE_HEIGHT nHeight
		DDF_SAME_DRAW			 // use prev params....
	);
	LOG(INFO) << " draw image " << nWidth << " *" << nHeight;
}

int ScaleImg(VideoCaptureCapability frameInfo, AVFrame *src_picture, int srcFormat, AVFrame *dst_picture, int nDstH, int nDstW, int dstFormat)
{
	int nSrcH = frameInfo.height;
	int nSrcW = frameInfo.width;

	struct SwsContext* m_pSwsContext;

	int nDstStride[3];
	switch (dstFormat)
	{
	case AV_PIX_FMT_YUV420P://for PIX_FMT_YUV420P, Planar
	{
		nDstStride[0] = nDstW;
		nDstStride[1] = nDstW / 2;
		nDstStride[2] = nDstW / 2;
	}
	break;
	case AV_PIX_FMT_YUYV422://for PIX_FMT_YUYV422, Packed
	{
		nDstStride[0] = 2 * nDstW;
		nDstStride[1] = 0;//useless
		nDstStride[2] = 0;//useless
	}
	break;
	case AV_PIX_FMT_RGB24:
	case AV_PIX_FMT_BGR24:
	{
		nDstStride[0] = nDstW;// *nDstH;// 3 * nDstW*nDstH;
		nDstStride[1] = nDstW;// 0;//useless
		nDstStride[2] = nDstW;// 0;//useless
	}
	break;
	default:
	{
		nDstStride[0] = nDstW;
		nDstStride[1] = nDstW / 2;
		nDstStride[2] = nDstW / 2;
	}
	}

	m_pSwsContext = sws_getContext(nSrcW, nSrcH, (AVPixelFormat)srcFormat, nDstW, nDstH, (AVPixelFormat)dstFormat, SWS_BICUBIC, NULL, NULL, NULL);
	if (NULL == m_pSwsContext) {
		LOG(INFO) << "ERROR: " << " ScaleImg,get SwsContext error!" << std::endl;
		return -1;
	}
//	sws_scale(m_pSwsContext, src_picture->data, src_picture->linesize, 0, frameInfo.height, dst_picture->data, nDstStride);
	sws_scale(m_pSwsContext, src_picture->data, src_picture->linesize, 0, frameInfo.height, dst_picture->data, nDstStride);
	sws_freeContext(m_pSwsContext);
	return 1;
}



void HDCRender::RenderFrame(unsigned char* pBuffer, int length, VideoCaptureCapability frameInfo) {
	
	enum AVPixelFormat src_pix_fmt = RawTypeToAVFormat(frameInfo.rawType), dst_pix_fmt = AV_PIX_FMT_BGR24;

	//uint8_t* rgbbuf = (uint8_t*)malloc(frameInfo.width * frameInfo.height * 3);
	AVFrame *pFrameYUV = av_frame_alloc();
	AVFrame *rgbFrame = av_frame_alloc();
	uint8_t *out_buffer = new uint8_t[avpicture_get_size(dst_pix_fmt, frameInfo.width, frameInfo.height)];
	avpicture_fill((AVPicture *)pFrameYUV, pBuffer, src_pix_fmt, frameInfo.width, frameInfo.height);
	
	avpicture_fill((AVPicture *)rgbFrame, out_buffer, dst_pix_fmt, frameInfo.width, frameInfo.height);



	SwsContext *sws_ctx = sws_getContext(
		frameInfo.width, frameInfo.height, src_pix_fmt,
		frameInfo.width, frameInfo.height, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);

	pFrameYUV->data[0] = pFrameYUV->data[0] + pFrameYUV->linesize[0] * (frameInfo.height - 1);
	pFrameYUV->linesize[0] *= -1;
	pFrameYUV->data[1] = pFrameYUV->data[1] + pFrameYUV->linesize[1] * (frameInfo.height / 2 - 1);
	pFrameYUV->linesize[1] *= -1;
	pFrameYUV->data[2] = pFrameYUV->data[2] + pFrameYUV->linesize[2] * (frameInfo.height / 2 - 1);
	pFrameYUV->linesize[2] *= -1;

	sws_scale(sws_ctx, pFrameYUV->data, pFrameYUV->linesize, 0, frameInfo.height, rgbFrame->data, rgbFrame->linesize);
	sws_freeContext(sws_ctx);
	PrintVideoData(rgbFrame->data[0], frameInfo.width, frameInfo.height);
	/*AVPixelFormat format = RawTypeToAVFormat(frameInfo.rawType);
	AVPicture rgbpic;
	AVFrame *picture;
	picture = av_frame_alloc();
	picture->format = AV_PIX_FMT_BGR24;
	picture->width = frameInfo.width;
	picture->height = frameInfo.height;
	//av_frame_get_buffer(picture, 32);
	//av_image_alloc(picture->data, picture->linesize, frameInfo.width, frameInfo.height, format, 32);
	avpicture_alloc((AVPicture *)picture, PIX_FMT_BGR24, frameInfo.width, frameInfo.height);

	avpicture_fill(&rgbpic, (uint8_t *)(pBuffer), format, frameInfo.width, frameInfo.height);
	//ScaleImg(frameInfo, (AVFrame*)&rgbpic, format, picture, frameInfo.height, frameInfo.width, AV_PIX_FMT_BGR24);
	PrintVideoData(picture->data[0], frameInfo.width, frameInfo.height);
	avpicture_free((AVPicture *)picture);
	av_frame_free(&picture);*/
}

