///58:46 HHD 004
#include <Windows.h>
#include <stdint.h>

#define internal static

static bool running;
static BITMAPINFO bitmapInfo;
static void* bitmapMemory;
static HDC deviceContext;
static int bitmapWidth = 0;
static int bitmapHeight = 0;
static int bytesPerPixel = 4;


internal void RenderGradient(int xOffset, int yOffset)
{
	int pitch = bitmapWidth * bytesPerPixel;
	uint8_t* row = (uint8_t*)bitmapMemory;
	for(int y = 0; y < bitmapHeight; y++)
	{
		uint32_t* pixel = (uint32_t*)row;
		for(int x = 0; x < bitmapWidth; x++)
		{
			uint8_t blue = (x + xOffset);
			uint8_t green = (y + yOffset);
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
	bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;

	int bitmapMemorySize = bitmapWidth * bitmapHeight * bytesPerPixel;
	bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}
internal void Win32UpdateWindow(HDC deviceContext, RECT clientRect, int x, int y, int width, int height)
{
	int windowWidth = clientRect.right - clientRect.left;
	int windowHeight = clientRect.bottom - clientRect.top;
	StretchDIBits(deviceContext,
		0, 0, bitmapWidth, bitmapHeight,
		0, 0, windowWidth, windowHeight,
		bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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

			static DWORD operation = BLACKNESS;
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
		/*	case WM_SETCURSOR:
			{
				SetCursor(0);
				break;
			}*/
		default:
		{
			result = DefWindowProc(window, message, wParam, lParam);
			break;
		}
	}
	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance,
	LPSTR lpCmdLine, int CmdShow)
{
	WNDCLASS windowClass = { 0 };
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowProc;
	windowClass.hInstance = instance;
	//windowClass.hIcon;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";
	if(RegisterClass(&windowClass))
	{
		HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "Basic Engine",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, instance, 0);
		if(windowHandle)
		{
			running = true;
			int xOffset = 0;
			int yOffset = 0;
			while(running)
			{
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if(message.message == WM_QUIT)
						running = false;
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}

				RenderGradient(xOffset, yOffset);

				HDC deviceContext = GetDC(windowHandle);
				RECT clientRect;
				GetClientRect(windowHandle, &clientRect);

				int width = clientRect.right - clientRect.left;
				int height = clientRect.bottom - clientRect.top;

				Win32UpdateWindow(deviceContext, clientRect, 0, 0, width, height);
				ReleaseDC(windowHandle, deviceContext);
				xOffset++;
				yOffset++;
			}
		}
		else
		{
			MessageBoxA(0, "Não foi possivel criar janela", "ERRO!", MB_OK | MB_ICONERROR);
		}
	}
	//MessageBoxA(0, "Um popup maneiro", "Popup", MB_OKCANCEL | MB_ICONEXCLAMATION);
	return 0;
}
