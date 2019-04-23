#include <Windows.h>
#include "types.h"
#include <cstdio>

#define internal static

static bool running;
static BITMAPINFO bitmapInfo;
static void* bitmapMemory;
static int bitmapWidth;
static int bitmapHeight;
static int bytesPerPixel = 4;

internal void RenderGradient(int blueOffset, int greenOffset)
{
	int width = bitmapWidth;
	int height = bitmapHeight;

	int pitch = width * bytesPerPixel;
	uint8* row = (uint8*)bitmapMemory;
	for(int y = 0; y < height; y++)
	{
		uint32* pixel = (uint32*)row;
		for(int x = 0; x < bitmapWidth; x++)
		{
			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);
			*pixel++ = ((green << 8) | blue);
		}
		row += pitch;
	}
}
internal void Win32ResizeDIBSection(int width, int height)
{
	if(bitmapMemory)
		VirtualFree(bitmapMemory, 0, MEM_RELEASE);

	bitmapWidth = width;
	bitmapHeight = height;

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = bitmapWidth;
	///Negativo para renderizar de da esquerda superior para baixo 
	bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;

	bitmapMemory = VirtualAlloc(0, bitmapWidth * bitmapHeight * bytesPerPixel, MEM_COMMIT, PAGE_READWRITE);
	RenderGradient(0, 0);

}
internal void Win32UpdateWindow(HDC deviceContext, RECT windowRect, int x, int y, int width, int height)
{
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;
	StretchDIBits(deviceContext, 0, 0, bitmapWidth, bitmapHeight,
		0, 0, windowWidth, windowHeight, bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32MainWindowsCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch(message)
	{
		case WM_SIZE:
		{
			RECT clientRect;
			GetClientRect(window, &clientRect);
			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;
			Win32ResizeDIBSection(width, height);

			break;
		}
		case WM_DESTROY:
		{
			running = false;
			break;
		}
		case WM_CLOSE:
		{
			running = false;
			break;
		}
		case WM_ACTIVATEAPP:
		{
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			RECT clientRect;
			GetClientRect(window, &clientRect);

			Win32UpdateWindow(deviceContext, clientRect, x, y, width, height);
			EndPaint(window, &paint);
			break;
		}
		default:
		{
			result = DefWindowProc(window, message, wParam, lParam);
			break;
		}
	}
	return result;
}

int CALLBACK  WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowsCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "BasicEngineWindowClass";
	if(RegisterClass(&windowClass))
	{
		HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "Basic Engine",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
		if(windowHandle)
		{
			running = 1;
			uint8 yOffset = 0, xOffset = 0;
			while(running)
			{
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if(message.message == WM_QUIT)
					{
						running = false;
					}
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				RenderGradient(xOffset, yOffset);
				HDC deviceContext = GetDC(windowHandle);
				RECT clientRect;
				GetClientRect(windowHandle, &clientRect);
				int windowWidth = clientRect.right - clientRect.left;
				int windowHeight = clientRect.bottom - clientRect.top;
				Win32UpdateWindow(deviceContext, clientRect, 0, 0, windowWidth, windowHeight);
				ReleaseDC(windowHandle, deviceContext);
				xOffset++;
				yOffset++;
			}
		}
		else
		{
			MessageBox(0, "Erro ao criar a janela!", "Não foi possivel criar a janela", MB_OK | MB_ICONERROR);
		}
	}
	return 0;
}