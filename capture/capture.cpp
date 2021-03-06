//capture.cpp: 定义应用程序的入口点。
//

#include "stdafx.h"
#include "capture.h"
#include <stdio.h>
#include <commdlg.h>
#include "DeviceInfoDS.h"
#include "string"
#include "iostream"
#include "VideoCaptureDS.h"
#include "HDCRender.h"
#include "logging.h"
using namespace google;

#define MAX_LOADSTRING 100
#pragma warning(suppress : 4996)
#pragma comment(lib, "glogd.lib")
// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

#define VIDEO_X_PIXEL 640
#define VIDEO_Y_PIXEL 480
#define EFILEURL 520
#define BLOADFILE 521
#define BPLAY 522
#define BPAUSE 523
#define BSTOP 524
#define BEXIT 525
#define ARRAY_LEN 256
#define SHOW_CACHEDURATION 257
#define ETHRESHOLD 258
#define BSETTHRESHOLD 259
#define VIEW 555

#define FRESH_hLadle_CacheDuration 2570

HINSTANCE appInstance;
HWND hVideoWin;
HWND hLadle_CacheDuration;
char Filename_CHAR[ARRAY_LEN];
char Edit_Last_Str[ARRAY_LEN];
char setting_url[ARRAY_LEN];

BOOL GetFileName(TCHAR *szFileName);
void TcharToChar(const TCHAR * tchar, char * _char);
WCHAR * charToWchar(const char *s);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	FLAGS_log_dir = "./log";
	google::InitGoogleLogging("capture");
	google::SetLogDestination(google::GLOG_INFO, (FLAGS_log_dir+"/INFO_").c_str());
	google::SetStderrLogging(google::GLOG_INFO);
	google::SetLogFilenameExtension("log_");
	FLAGS_colorlogtostderr = true;  // Set log color
	FLAGS_logbufsecs = 0;  // Set log output speed(s)
	FLAGS_max_log_size = 1024;  // Set max log file size
	FLAGS_stop_logging_if_full_disk = true;  // If disk is full
	LOG(INFO) << "1111111111";
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	appInstance = hInstance;
    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CAPTURE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CAPTURE));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAPTURE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CAPTURE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	//#pragma warning(disable : 4996)
	//freopen("win32playerdemo.log", "w", stdout);
	/*FILE *file_setting = fopen("setting.ini", "r+");
	if (file_setting) {
		fgets(setting_url, ARRAY_LEN, file_setting);
		fclose(file_setting);
		file_setting = NULL;
	}*/
    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
#include"Mp4Writer.h"
class FrameCaptureObserver: public VideoCaptureObserver {
public:
	FrameCaptureObserver(HWND wnd) {
		wnd_ = wnd;
		render_ = new HDCRender(wnd);
		render_->InitDCParam();
		is_first_ = true;
		started_ = false;
	}
	void StartRecoder() {	
		started_ = true;
	}
	void StopRecoder() {
		started_ = false;
		mp4_writer_.CloseMp4();
	}
	virtual void OnReceiveFrame(unsigned char* pBuffer, int32_t length, VideoCaptureCapability frameInfo) {
		if (is_first_ && started_) {
			mp4_writer_.CreateMp4("E:\\c++\\capture\\capture\\1234556.mp4", frameInfo);
			is_first_ = false;
		}
		else if (started_){
			mp4_writer_.WriteRawdata((char*)pBuffer, length);
		}
		render_->RenderFrame(pBuffer, length, frameInfo);
	}
private:
	bool started_;
	bool is_first_;
	HWND wnd_;
	HDCRender* render_;
	Mp4Writer mp4_writer_;
};
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
#define ARRAY_LEN 256
VideoCaptureDS *ds = NULL;
FrameCaptureObserver *observer;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc; //Handle Device Context
	PAINTSTRUCT ps;
	RECT rect;
	BOOL error;
	HWND hEdit_URL;
	HWND hEdit_Threshold;
	HWND hButton_SetThreshold;
	HWND hButton_LoadFile;
	HWND hButton_Play;
	HWND hButton_Pause;
	HWND hButton_Stop;
	HWND hButton_Exit;
	//HWND hVideoWin;

	int default_maxbuffer_size = 1000;//ms
	long int default_speed_threshold_ = 1000;//ms
	long int cache_duration_ms = 0;
	WCHAR *cache_duration_sw = NULL;

	int Button_Width = 70;
	int Button_Height = 25;
	int Button_XPos = 50;
	int Button_YPos = 550;
	int Button_Rang = 100;

	TCHAR Filename_TCHAR[ARRAY_LEN] = { 0 };
	wchar_t Filename_WCHAR_T[ARRAY_LEN] = { 0 };
	char tmp_Filename_CHAR[ARRAY_LEN] = { 0 };
	char cache_duration_char[ARRAY_LEN] = { 0 };
	wchar_t Threshold_WCHAR_T[ARRAY_LEN] = { 0 };
	char tmp_Threshold_CHAR[ARRAY_LEN] = { 0 };
    switch (message)
    {
	case WM_CREATE:
	{
		//The window for display video frame.
		//hVideoWin = CreateWindow(TEXT("Win32 Player Demo"),
		//	TEXT("Child window"),
		//	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		//	0,//CW_USEDEFAULT,
		//	0,//CW_USEDEFAULT,
		//	400,///VIDEO_X_PIXEL,
		//	400,//VIDEO_Y_PIXEL,
		//	hWnd,//parent window handle.
		//	(HMENU)VIEW,
		//	appInstance,
		//	NULL);

		hVideoWin = (HWND)CreateWindow(TEXT("EDIT"),
			TEXT("view"),
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, (int)(500.0*16/9), 500,
			hWnd,
			(HMENU)VIEW,
			appInstance, NULL);

		LOG(INFO) <<" create video wnd ERROR RRRRRRRRRRRRR " <<hVideoWin;
		//hVideoWin = hWnd;

		hButton_LoadFile = (HWND)CreateWindow(TEXT("Button"),
			TEXT("LoadFile"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos, Button_YPos, Button_Width, Button_Height,
			hWnd,
			(HMENU)BLOADFILE,
			appInstance,
			NULL);
		hButton_Play = (HWND)CreateWindow(TEXT("Button"),
			TEXT("Play"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos + Button_Rang, Button_YPos, Button_Width, Button_Height,
			hWnd,
			(HMENU)BPLAY,
			appInstance,
			NULL);
		hButton_Pause = (HWND)CreateWindow(TEXT("Button"),
			TEXT("Pause"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos + Button_Rang * 2, Button_YPos, Button_Width, Button_Height,
			hWnd,
			(HMENU)BPAUSE,
			appInstance,
			NULL);
		hButton_Stop = (HWND)CreateWindow(TEXT("Button"),
			TEXT("Stop"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos + Button_Rang * 3, Button_YPos, Button_Width, Button_Height,
			hWnd,
			(HMENU)BSTOP,
			appInstance,
			NULL);
		hButton_Exit = (HWND)CreateWindow(TEXT("Button"),
			TEXT("Exit"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos + Button_Rang * 4, Button_YPos, Button_Width, Button_Height,
			hWnd,
			(HMENU)BEXIT,
			appInstance,
			NULL);

		hButton_SetThreshold = (HWND)CreateWindow(TEXT("Button"),
			TEXT("Threshold"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			Button_XPos, Button_YPos + 40, Button_Width, Button_Height,
			hWnd,
			(HMENU)BSETTHRESHOLD,
			appInstance,
			NULL);

		hEdit_Threshold = (HWND)CreateWindow(TEXT("EDIT"),
			TEXT("1000ms"),
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
			Button_XPos + Button_Width + 3, Button_YPos + 40, Button_Width, Button_Height,
			hWnd,
			(HMENU)ETHRESHOLD,
			appInstance,
			NULL);

		SetTimer(hWnd, FRESH_hLadle_CacheDuration, 300, NULL);//Set Timer
		hLadle_CacheDuration = (HWND)CreateWindow(TEXT("STATIC"),
			TEXT("0.0s"),
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			Button_XPos + Button_Rang * 2, Button_YPos + 40, Button_Width, Button_Height,
			hWnd,
			(HMENU)SHOW_CACHEDURATION,
			appInstance, NULL);

		hEdit_URL = (HWND)CreateWindow(TEXT("EDIT"),
			charToWchar(setting_url),
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
			Button_XPos, Button_YPos + 80, Button_Width * 7, Button_Height,
			hWnd,
			(HMENU)EFILEURL,
			appInstance,
			NULL);

		break;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
			case BLOADFILE:
				if ( HIWORD(wParam) == BN_CLICKED)
				{
					error = GetFileName(Filename_TCHAR);
					if (error) {
						if (strlen(Filename_CHAR) > 0) {
							
						}						
						TcharToChar(Filename_TCHAR, Filename_CHAR);
						MessageBox(hWnd, Filename_TCHAR, TEXT("LoadFile"), MB_OK);
						
						//df.DisplayCaptureSettingsDialogBox(st2.c_str(), "test", hWnd, 0, 0);
						//MessageBox(hWnd, charToWchar(st1.c_str()), TEXT("LoadFile"), MB_OK);

					}
					else {
						MessageBox(hWnd, TEXT("Load file error!"), TEXT("LoadFile"), MB_OK);
					}
				}
				break;
            //case IDM_ABOUT:
            //    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            //    break;
            //case IDM_EXIT:
            //    DestroyWindow(hWnd);
            //    break;
            //default:
            //    return DefWindowProc(hWnd, message, wParam, lParam);

			case BPLAY:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					DeviceInfoDS df(12);
					df.Init();

					std::string st1;
					std::string st2;
					std::string st3;
					df.GetDeviceInfo(0, &st1, &st2, &st3);
					df.CreateCapabilityMap(st2.c_str());
					ds = new VideoCaptureDS(0, hVideoWin);
					LOG(INFO) << " hVideoWin = " << hVideoWin;
					observer = new FrameCaptureObserver(hVideoWin);
					ds->Init(0, st2.c_str(), observer);
					observer->StartRecoder();
					VideoCaptureCapability capability;

					capability.width = 1280;
					capability.height = 720;
					capability.maxFPS = 25;
					capability.expectedCaptureDelay = 0;
					capability.rawType = kVideoYUY2;// kVideoI420;
					capability.codecType = kVideoCodecUnknown;
					capability.interlaced = false;
					LOG(INFO) << "to start capture ===";
					ds->StartCapture(capability);
				}
				break;
			case BSTOP:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					observer->StopRecoder();
				}
				break;
            } // switch
        }// WM_COMMAND
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
			GetClientRect(hWnd, &rect);
			//DrawText(hdc, TEXT("The window display video frame."), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
WCHAR * charToWchar(const char *s)
{
	int w_nlen = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
	WCHAR *ret;
	ret = (WCHAR *)malloc(sizeof(WCHAR) * w_nlen);
	memset(ret, 0, sizeof(ret));
	MultiByteToWideChar(CP_ACP, 0, s, -1, ret, w_nlen);
	return ret;
}

BOOL GetFileName(TCHAR *szFileName)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = 0;
	ofn.lpstrDefExt = 0;
	ofn.lpstrFile = szFileName;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn) == NULL)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void TcharToChar(const TCHAR * tchar, char * _char)
{
	int iLength;

	iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}