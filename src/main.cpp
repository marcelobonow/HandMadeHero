/// HMHD 006 41:33
#include <Windows.h>
#include <xinput.h>
#include "types.h"


#define internal static

struct Win32OffScreenBuffer
{
	BITMAPINFO bitmapInfo;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};
static bool running;
static Win32OffScreenBuffer globalBackBuffer;

struct Win32WindowDimension
{
	int width;
	int height;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pstate)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)


typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(0);
}
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(0);
}

static x_input_get_state* DyXInputGetState = XInputGetStateStub;
static x_input_set_state* DyXInputSetState = XInputSetStateStub;

#define XInputGetState DyXInputGetState
#define XInputSetState DyXInputSetState

static void Win32LoadXInput(void)
{
	HMODULE xInputLibrary = LoadLibrary("xinput1_3.dll");
	if(xInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
	}
}

static Win32WindowDimension Win32GetWindowDimension(HWND window)
{
	Win32WindowDimension result;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}


internal void RenderGradient(Win32OffScreenBuffer buffer, int blueOffset, int greenOffset)
{
	uint8* row = (uint8*)buffer.memory;
	for(int y = 0; y < buffer.height; y++)
	{
		uint32* pixel = (uint32*)row;
		for(int x = 0; x < buffer.width; x++)
		{
			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);
			*pixel++ = ((green << 8) | blue);
		}
		row += buffer.pitch;
	}
}
internal void Win32ResizeDIBSection(Win32OffScreenBuffer* buffer, int width, int height)
{
	if(buffer->memory)
		VirtualFree(buffer->memory, 0, MEM_RELEASE);

	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;

	buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
	buffer->bitmapInfo.bmiHeader.biWidth = buffer->width;
	///Negativo para renderizar de da esquerda superior para baixo 
	buffer->bitmapInfo.bmiHeader.biHeight = -buffer->height;
	buffer->bitmapInfo.bmiHeader.biPlanes = 1;
	buffer->bitmapInfo.bmiHeader.biBitCount = 32;
	buffer->memory = VirtualAlloc(0, buffer->width * buffer->height * buffer->bytesPerPixel, MEM_COMMIT, PAGE_READWRITE);

	buffer->pitch = width * buffer->bytesPerPixel;
}
internal void Win32DisplayBufferToWindow(HDC deviceContext, int windowWidth, int windowHeight, Win32OffScreenBuffer buffer, int x, int y, int width, int height)
{
	StretchDIBits(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer.width, buffer.height,
		buffer.memory, &buffer.bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32MainWindowsCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch(message)
	{
		case WM_SIZE:
		{
			//Win32WindowDimension dimension = Win32GetWindowDimension(window);
			//Win32ResizeDIBSection(&globalBackBuffer, dimension.width, dimension.height);
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

			Win32WindowDimension dimension = Win32GetWindowDimension(window);

			Win32DisplayBufferToWindow(deviceContext, dimension.width, dimension.height, globalBackBuffer, x, y, width, height);
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
	Win32LoadXInput();

	WNDCLASS windowClass = {};

	windowClass.style = CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = Win32MainWindowsCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "BasicEngineWindowClass";
	if(RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(0, windowClass.lpszClassName, "Basic Engine",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
		if(window)
		{
			running = 1;
			uint8 yOffset = 0, xOffset = 0;

			Win32WindowDimension dimension = Win32GetWindowDimension(window);
			Win32ResizeDIBSection(&globalBackBuffer, dimension.width, dimension.height);
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
				for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
				{
					XINPUT_STATE controllerState;
					if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						///Controller is plugged
					}
				}
				RenderGradient(globalBackBuffer, xOffset, yOffset);
				Win32WindowDimension dimension = Win32GetWindowDimension(window);
				HDC deviceContext = GetDC(window);
				RECT clientRect;
				GetClientRect(window, &clientRect);
				int windowWidth = clientRect.right - clientRect.left;
				int windowHeight = clientRect.bottom - clientRect.top;

				Win32DisplayBufferToWindow(deviceContext, windowWidth, windowHeight, globalBackBuffer, 0, 0, windowWidth, windowHeight);
				ReleaseDC(window, deviceContext);
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