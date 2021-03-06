// Demo.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../heic2jpeg/Heic2jpegExport.h"
#include <windows.h>
// #ifdef _WIN64
// #ifdef _DEBUG
// #pragma comment(lib,"../Output/x64/Debug/heic2jpeg.lib")
// #else
// #pragma comment(lib,"../Output/x64/Release/heic2jpeg.lib")
// #endif
// #else
// #ifdef _DEBUG
// #pragma comment(lib,"../Output/Win32/Debug/heic2jpeg.lib")
// #else
// #pragma comment(lib,"../Output/Win32/Release/heic2jpeg.lib")
// #endif
// #endif

typedef ITL_ERT(__cdecl *_EuHeic2jpeg)(const wchar_t *, const wchar_t *);
typedef ITL_ERT(__cdecl *_EuHeic2Thumbnail)(const wchar_t *, const wchar_t *);

int main()
{
	//const wchar_t* inputFileName = L"C:\\Users\\admin\\Desktop\\IMG_0034.HEIC";
	const wchar_t* inputFileName = L"E:\\HEIC\\pic_heic\\IMG_0038.HEIC";
 	const wchar_t* outputFileName = L"C:\\Users\\admin\\Desktop\\IMG_0038.jpeg";
	const wchar_t* outputFileNameThumbnail = L"C:\\Users\\admin\\Desktop\\IMG_0038_Thumbnail.jpeg";

	_EuHeic2jpeg			    m_pfnEuHeic2jpeg = NULL;
	_EuHeic2Thumbnail			    m_pfnEuHeic2Thumbnail = NULL;

	 #ifdef _WIN64
	 #ifdef _DEBUG
	HMODULE m_hDll = ::LoadLibrary(L"../Output/x64/Debug/heic2jpeg.dll");
	 #else
	HMODULE m_hDll = ::LoadLibrary(L"../Output/x64/Release/heic2jpeg.dll");
	 #endif
	 #else
	 #ifdef _DEBUG
	HMODULE m_hDll = ::LoadLibrary(L"../Output/Win32/Debug/heic2jpeg.dll");
	 #else
	HMODULE m_hDll = ::LoadLibrary(L"../Output/Win32/Release/heic2jpeg.dll");
	 #endif
	 #endif
	if (NULL != m_hDll)
	{
		m_pfnEuHeic2jpeg = (_EuHeic2jpeg)GetProcAddress(m_hDll, "EuHeic2jpeg");
		m_pfnEuHeic2Thumbnail = (_EuHeic2Thumbnail)GetProcAddress(m_hDll, "EuHeic2Thumbnail");
		if (m_pfnEuHeic2Thumbnail)
		{
			ITL_ERT bret = m_pfnEuHeic2Thumbnail(inputFileName, outputFileNameThumbnail);
		}
		if (m_pfnEuHeic2jpeg)
		{
			ITL_ERT bret = m_pfnEuHeic2jpeg(inputFileName, outputFileName);
		}
	}
	if(m_hDll)
	{
		FreeLibrary(m_hDll);
		m_hDll = NULL;
	}

	system("pause");
    return 0;
}

