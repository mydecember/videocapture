#pragma once
#include <olectl.h>
#include <initguid.h>
#include<vfw.h>
#include "Comm.h"

class HDCRender
{
public:
	HDCRender(HWND newhand = 0);
	~HDCRender();

	void InitDCParam();//初始化绘图参数
	void PrintVideoData(BYTE *data, int nWidth, int nHeight);//实际上，我们这里可以传入,实际的长、宽
	void setWndSize(int width, int height) { m_wndWidth = width; m_wndHeight = height; }
	void setImageSize(int width, int height) { m_IMAGE_WIDTH = width; m_IMAGE_HEIGHT = height; }
	void RenderFrame(unsigned char* pBuffer, int length, VideoCaptureCapability frameInfo);
private:
	BITMAPINFO m_bmpinfo;
	int local_wnd_x, local_wnd_y, local_wnd_xc, local_wnd_yc;
	HDC m_hdc;
	HDRAWDIB hdib;
	int m_wndWidth, m_wndHeight;
	int m_IMAGE_WIDTH, m_IMAGE_HEIGHT;
	int haveInit;

	RECT m_winRect, m_clientRect;

	HWND handl;
	HWND handlNew;
	int m_wndChange;
	int m_stat;
};

