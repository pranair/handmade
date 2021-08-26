#include <stdint.h>
#include <math.h>

#if HANDMADE_SLOW
#define Assert(Expr) if(!(Expr)) {*(int *)0 = 1;}
#else
#define Assert(Expr) 
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32 bool32;
typedef float real32;
typedef double real64;

#define Pi32 3.14159265359
#define global_variable static
#define local_persist static
#define internal static

#include "handmade.cpp"

#include <windows.h>
#include <dsound.h>
#include <stdio.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "win32_handmade.h"

global_variable IAudioClient* GlobalSoundClient;
global_variable IAudioRenderClient* GlobalSoundRenderClient;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable bool GlobalRunning = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int GlobalXOffset;
global_variable int GlobalYOffset;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

internal void
Win32InitWASAPI(int32 SamplesPerSecond, int32 BufferSizeInSamples) {
	if (FAILED(CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY))) {
		//Assert(!"Error");
	}

	IMMDeviceEnumerator* Enumerator;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator)))) {
		//Assert(!"Error");
	}

	IMMDevice* Device;
	if (FAILED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device))) {
		//Assert(!"Error");
	}

	if (FAILED(Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (LPVOID*)&GlobalSoundClient))) {
		//Assert(!"Error");
	}

	WAVEFORMATEXTENSIBLE WaveFormat;

	WaveFormat.Format.cbSize = sizeof(WaveFormat);
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	WaveFormat.Format.wBitsPerSample = 16;
	WaveFormat.Format.nChannels = 2;
	WaveFormat.Format.nSamplesPerSec = (DWORD)SamplesPerSecond;
	WaveFormat.Format.nBlockAlign = (WORD)(WaveFormat.Format.nChannels * WaveFormat.Format.wBitsPerSample / 8);
	WaveFormat.Format.nAvgBytesPerSec = WaveFormat.Format.nSamplesPerSec * WaveFormat.Format.nBlockAlign;
	WaveFormat.Samples.wValidBitsPerSample = 16;
	WaveFormat.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
	WaveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

	REFERENCE_TIME BufferDuration = 10000000ULL * BufferSizeInSamples / SamplesPerSecond; // buffer size in 100 nanoseconds
	if (FAILED(GlobalSoundClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST, BufferDuration, 0, &WaveFormat.Format, nullptr))) {
		//Assert(!"Error");
	}

	HRESULT er = GlobalSoundClient->GetService(IID_PPV_ARGS(&GlobalSoundRenderClient));
	if (FAILED(er)) {
		//Assert(!"Error");
	}

	UINT32 SoundFrameCount;
	if (FAILED(GlobalSoundClient->GetBufferSize(&SoundFrameCount))) {
		//Assert(!"Error");
	}

	// Check if we got what we requested (better would to pass this value back as real buffer size)
	//Assert(BufferSizeInSamples <= (int32)SoundFrameCount);
}



internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput) {
	BYTE* SoundBufferData;
	if (SUCCEEDED(GlobalSoundRenderClient->GetBuffer((UINT32)SoundOutput->BufferSize, &SoundBufferData))) {
		uint8 *SampleOut = (uint8 *)SoundBufferData;
		for(int i = 0; i < SoundOutput->BufferSize; i++) {
			*SampleOut++ = 0;
		}
		
		GlobalSoundRenderClient->ReleaseBuffer((UINT32)SoundOutput->BufferSize, 0);
	}
}



internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, int SamplesToWrite, game_sound_buffer *SourceBuffer) {
	BYTE* SoundBufferData;
	if (SUCCEEDED(GlobalSoundRenderClient->GetBuffer((UINT32)SamplesToWrite, &SoundBufferData)))
		{
			int16* SourceSample = SourceBuffer->Samples;
			int16* DestSample = (int16*)SoundBufferData;
			for(int SampleIndex = 0;
				SampleIndex < SamplesToWrite;
				++SampleIndex)
				{
					*DestSample++ = *SourceSample++; 
					*DestSample++ = *SourceSample++; 
					++SoundOutput->RunningSampleIndex;
				}

			GlobalSoundRenderClient->ReleaseBuffer((uint32)SamplesToWrite, 0);
		}
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {
	if(Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = Buffer->BytesPerPixel * 8;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapSize = Buffer->Width * Buffer->Height * Buffer->BytesPerPixel;

	Buffer->Memory = VirtualAlloc(0, BitmapSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(
				  HDC DeviceContext,
				  int WindowWidth,
				  int WindowHeight,
				  win32_offscreen_buffer *Buffer
				  ) {
	StretchDIBits(
				  DeviceContext,
				  0, 0, WindowWidth, WindowHeight, // dest
				  0, 0, Buffer->Width, Buffer->Height, // src
				  Buffer->Memory,
				  &Buffer->Info,
				  DIB_RGB_COLORS,
				  SRCCOPY
				  );
}

internal void
Win32GetKeyState(game_keyboard_input *Input) {
	SHORT IsUpDown = GetAsyncKeyState(VK_UP);
	SHORT IsDownDown = GetAsyncKeyState(VK_DOWN);
	if (GetKey(IsDownDown)) {
		Input->IsDown = true;
	}
	if (GetKey(IsUpDown)) {
		Input->IsUp = true;
	}
}

internal LRESULT CALLBACK
Win32MainWindowCallback(
						HWND Window,
						UINT Message,
						WPARAM WParam,
						LPARAM LParam
						) {
	LRESULT Result = 0;
	switch(Message) {
	case WM_ACTIVATEAPP: {
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_CLOSE: {
		GlobalRunning = false;
		OutputDebugStringA("WM_CLOSE\n");
	} break;

	case WM_DESTROY: {
		OutputDebugStringA("WM_DESTROY\n");
	} break;

	case WM_SYSKEYUP:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_KEYDOWN: {
		uint32 VKCode = WParam;
		bool32 WasDown = LParam & (1 << 30);
		bool32 IsDown = LParam & (1 << 31);

		int diff = 20;

		switch(VKCode) {
		case VK_F4: {
			bool32 IsAltDown = LParam & (1 << 29);
			if(IsAltDown) {
				GlobalRunning = false;
			}
		} break;

		case VK_UP: {
			//GlobalYOffset += diff;
		} break;

		case VK_DOWN: {
			//GlobalYOffset -= diff;
		} break;

		case VK_LEFT: {
			//GlobalXOffset -= diff;
		} break;

		case VK_RIGHT: {
			//GlobalXOffset += diff;
		} break;
		}

	} break;

	case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct = {};
			HDC DeviceContext = BeginPaint(Window, &PaintStruct);
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32UpdateWindow(
							  DeviceContext,
							  Dimension.Width,
							  Dimension.Height,
							  &GlobalBackBuffer
							  );
			EndPaint(Window, &PaintStruct);
		} break;

	case WM_SIZE:
		{
			OutputDebugStringA("WM_SIZE\n");
		} break;

	default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return Result;
}

int CALLBACK
WinMain(
		HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR     CmdLine,
		int       ShowCmd
		) {
	LARGE_INTEGER CounterPerSecond;
	QueryPerformanceFrequency(&CounterPerSecond);

	WNDCLASS WindowClass = {};

	// TODO: Check if we need these
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;

	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if(RegisterClass(&WindowClass)) {
		HWND Window = CreateWindowEx(
									 0,
									 "HandmadeHeroWindowClass",
									 "Handmade Hero",
									 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 0,
									 0,
									 Instance,
									 0
									 );

		if(Window) {
			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			SoundOutput.BytesPerSample = 4;
			SoundOutput.BufferSize = ((SoundOutput.SamplesPerSecond) * SoundOutput.BytesPerSample)/15;
			SoundOutput.RunningSampleIndex = 0;

			Win32InitWASAPI(SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);
			//Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.BufferSize);
			//Win32ClearSoundBuffer(&SoundOutput);
			GlobalSoundClient->Start();
			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.BufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

			Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
			
			//ShowWindow(Window, ShowCmd);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (uint64*) Terabytes((uint64)2);
#else
				
			LPVOID BaseAddress = 0;
#endif
				
			game_memory Memory = {};
			//Memory.PermenantStorageSize = 1024*1024*500;
			Memory.PermenantStorageSize = Megabytes(64);
			Memory.TempStorageSize = Megabytes(60);
			uint64 TotalSize = Memory.TempStorageSize + Memory.PermenantStorageSize;
			Memory.PermenantStorage = VirtualAlloc(BaseAddress,
												   TotalSize,
												   MEM_COMMIT|MEM_RESERVE,
												   PAGE_READWRITE);
			Memory.TempStorage = (uint8 *) (Memory.PermenantStorage) + Memory.PermenantStorageSize;
	
			LARGE_INTEGER LastCounter;
			uint64 LastCycleCounter = __rdtsc();
			QueryPerformanceCounter(&LastCounter);


			if (Samples && Memory.PermenantStorage && Memory.TempStorage) {
				while(GlobalRunning) {
					MSG Message = {};

					while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}

					int SamplesToWrite = 0;
					UINT32 SoundPaddingSize;
					if (SUCCEEDED(GlobalSoundClient->GetCurrentPadding(&SoundPaddingSize))) {
						SamplesToWrite = (int)(SoundOutput.BufferSize - SoundPaddingSize);
						if (SamplesToWrite > SoundOutput.LatencySampleCount) {
							SamplesToWrite = SoundOutput.LatencySampleCount;
						}
					}

					game_sound_buffer SoundBuffer = {};
					SoundBuffer.SampleCount = SamplesToWrite;
					SoundBuffer.Samples = Samples;
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;

					game_keyboard_input Input = {};
					Win32GetKeyState(&Input);
					GameUpdateAndRender(&Buffer, &Input, &SoundBuffer, &Memory);

					Win32FillSoundBuffer(&SoundOutput, SamplesToWrite, &SoundBuffer);


					HDC DeviceContext = GetDC(Window);
					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32UpdateWindow(
									  DeviceContext,
									  Dimension.Width,
									  Dimension.Height,
									  &GlobalBackBuffer
									  );
					ReleaseDC(Window, DeviceContext);

					uint64 CurrentCycleCounter = __rdtsc();
					LARGE_INTEGER CurrentCounter;
					QueryPerformanceCounter(&CurrentCounter);

					int64 CounterElapsed = CurrentCounter.QuadPart - LastCounter.QuadPart;
					uint64 CycleElapsed = CurrentCycleCounter - LastCycleCounter;

					real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)CounterPerSecond.QuadPart;
					real32 FPS = (real32)CounterPerSecond.QuadPart / (real32)CounterElapsed;
					real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);

# if 0
					char OutputBuffer[256];
					sprintf(OutputBuffer, "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(OutputBuffer);
#endif

					LastCounter = CurrentCounter;
					LastCycleCounter = CurrentCycleCounter;
				}
			} else {
				
			}
		} else {
			// TODO: Logging
		}
	} else {
		// TDOO: Logging
	}

	return 0;
}
