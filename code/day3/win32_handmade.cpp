#include <stdint.h>
#include <math.h>
#include <cmath>

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
#include <avrt.h>
#include <dsound.h>
#include <stdio.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <transact.h>
#include "win32_handmade.h"

global_variable IAudioClient3* GlobalSoundClient;
global_variable IAudioClock* GlobalAudioClock;
global_variable IAudioRenderClient* GlobalSoundRenderClient;

global_variable int SkipIndex;

global_variable bool GlobalRunning = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int GlobalXOffset;
global_variable int GlobalYOffset;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;


internal void
Win32InitWASAPI(int32 SamplesPerSecond, int32 BufferSizeInSamples) {
	if (FAILED(CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY))) {
		Assert(!"Error");
	}

	IMMDeviceEnumerator* Enumerator;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&Enumerator)))) {
		Assert(!"Error");
	}

	IMMDevice* Device;
	if (FAILED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device))) {
		Assert(!"Error");
	}

	if (FAILED(Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (LPVOID*)&GlobalSoundClient))) {
		Assert(!"Error");
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
		Assert(!"Error");
	}

	if (FAILED(GlobalSoundClient->GetService(IID_PPV_ARGS(&GlobalSoundRenderClient)))) {
		Assert(!"Error");
	}

	UINT32 SoundFrameCount;
	if (FAILED(GlobalSoundClient->GetBufferSize(&SoundFrameCount))) {
		Assert(!"Error");
	}

	GlobalSoundClient->GetService(__uuidof(IAudioClock), (void **)&GlobalAudioClock);

	// Check if we got what we requested (better would to pass this value back as real buffer size)
	Assert(BufferSizeInSamples <= (int32)SoundFrameCount);
}



/*
internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput) {
	BYTE* SoundBufferData;
	if (SUCCEEDED(GlobalSoundRenderClient->GetBuffer((UINT32)SoundOutput->BufferSize, &SoundBufferData))) {
		uint8 *SampleOut = (uint8 *)SoundBufferData;
		for(uint8 i = 0; i < SoundOutput->BufferSize; i++) {
			*SampleOut++ = 0;
		}
		
		GlobalSoundRenderClient->ReleaseBuffer((UINT32)SoundOutput->BufferSize, 0);
	}
}
*/


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
					//++SoundOutput->RunningSampleIndex;
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
	Buffer->Info.bmiHeader.biBitCount = (WORD) (Buffer->BytesPerPixel * 8);
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapSize = Buffer->Width * Buffer->Height * Buffer->BytesPerPixel;

	Buffer->Memory = VirtualAlloc(0, BitmapSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal debug_read_entire_file
DEBUGPlatformReadEntireFile(char *Filename) {
	debug_read_entire_file Result = {};
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ,
									FILE_SHARE_READ, 0,
									OPEN_EXISTING, 0, 0);
	void *Memory = 0;
	if (FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER Size;
		if (GetFileSizeEx(FileHandle, &Size)) {
			DWORD FileSize = SafeConvertUInt64(Size.QuadPart);
			DWORD BytesRead;
			Memory = VirtualAlloc(0, FileSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			if (ReadFile(FileHandle, Memory, FileSize, &BytesRead, 0) &&
				(FileSize == BytesRead)) {
				Result.Content = Memory;
				Result.ContentSize = FileSize;
			} else {
				DEBUGPlatformFreeFileMemory(Memory);
				Memory = 0;  
			}
			CloseHandle(FileHandle);
		} else {
		  
		}
	} else {
	  
	}
	return Result;
}

internal void
DEBUGPlatformFreeFileMemory(void *Filememory) {
	if (Filememory) {
		VirtualFree(Filememory, 0, MEM_RELEASE);
	}
}

internal bool
DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory) {
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE,
									0, 0,
									CREATE_ALWAYS, 0, 0);
	bool Result = false;
	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
			Result = (BytesWritten == MemorySize);
		} else {
			 
		}
		CloseHandle(FileHandle);
	} else {
	  
	}
	return Result;
}


internal void
Win32UpdateWindow(HDC DeviceContext,
				  int WindowWidth,
				  int WindowHeight,
				  win32_offscreen_buffer *Buffer) {
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
	SHORT IsLeftDown = GetAsyncKeyState(VK_LEFT);
	SHORT IsRightDown = GetAsyncKeyState(VK_RIGHT);
	SHORT IsQDown = GetAsyncKeyState('Q');
	SHORT IsEDown = GetAsyncKeyState('E');

	if (GetKey(IsDownDown)) {
		Input->IsDown = true;
	}
	if (GetKey(IsUpDown)) {
		Input->IsUp = true;
	}
	if (GetKey(IsLeftDown)) {
		Input->IsLeft = true;
	}
	if (GetKey(IsRightDown)) {
		Input->IsRight = true;
	}
	if (GetKey(IsQDown)) {
		Input->IsQ = true;
	}
	if (GetKey(IsEDown)) {
		Input->IsE = true;
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
		GlobalSoundClient->Stop();
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
		uint32 VKCode = (WORD) WParam;
		bool32 WasDown = LParam & (1 << 30);
		bool32 IsDown = LParam & (1 << 31);

		int diff = 20;

		switch(VKCode) {
		case VK_F4: {
			bool32 IsAltDown = LParam & (1 << 29);
			if(IsAltDown) {
				GlobalSoundClient->Stop();
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

		case VK_ESCAPE: {
			if (IsDown) {
				GlobalSoundClient->Stop();
				GlobalRunning = false;
			}
			//GlobalXOffset += diff;
		} break;
		}

	} break;

	case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct = {};
			HDC DeviceContext = BeginPaint(Window, &PaintStruct);
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32UpdateWindow(DeviceContext,
							  Dimension.Width,
							  Dimension.Height,
							  &GlobalBackBuffer);
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

global_variable LARGE_INTEGER GlobalPerfFreq;

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
	real32 result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfFreq.QuadPart);
	return result;
}

inline LARGE_INTEGER
Win32GetWallClockTime() {
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR     CmdLine,
		int       ShowCmd) {

	// TODO: is this needed?
	//SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
	DWORD TaskIndex = 0;
	HANDLE MultimediaHandle = AvSetMmThreadCharacteristics("Pro Audio", &TaskIndex);
	AvSetMmThreadPriority(MultimediaHandle, AVRT_PRIORITY_HIGH);


	LARGE_INTEGER CounterPerSecond;
	QueryPerformanceFrequency(&CounterPerSecond);
	QueryPerformanceFrequency(&GlobalPerfFreq);

	/*
	TIMECAPS PTC = {};
	timeGetDevCaps(&PTC, sizeof(PTC));
	UINT DesiredSchedulerMS = PTC.wPeriodMin; // 1 ms
	bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
	*/


	bool IsAudioPlaying = false;
	uint32 LastPadding = 0;
	int MoniterRefreshRate = 60;
	int GameRefreshRate = MoniterRefreshRate / 2;
	real32 TargetSecondsPerFrame = 1.0f / (real32) GameRefreshRate;

	WNDCLASS WindowClass = {};

	// TODO: Check if we need these
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;

	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if(RegisterClass(&WindowClass)) {
		HWND Window = CreateWindowEx(0,
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
									 0);

		if(Window) {
			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = 4;
			SoundOutput.LatencySampleCount = (SoundOutput.SamplesPerSecond / GameRefreshRate); 
			SoundOutput.BufferSize = SoundOutput.SamplesPerSecond ;

			Win32InitWASAPI(SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);


			REFERENCE_TIME DefaultDevicePeriod;
			GlobalSoundClient->GetDevicePeriod(&DefaultDevicePeriod, NULL);

			// FIXME: TEST CODE
			uint32 BufferSizeInFrames;
			GlobalSoundClient->GetBufferSize(&BufferSizeInFrames);

			//Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.BufferSize);
			//Win32ClearSoundBuffer(&SoundOutput);
			//GlobalSoundClient->Start();
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

			LARGE_INTEGER StartTime = Win32GetWallClockTime();

			if (Samples && Memory.PermenantStorage && Memory.TempStorage) {
				while(GlobalRunning) {
					MSG Message = {};

					while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}

					real32 ExpectedPlayPosition = (real32) Win32GetSecondsElapsed(LastCounter,
																				  Win32GetWallClockTime());
					UINT32 SoundPaddingSize;
					int WriteSampleOffset = 0;
					uint32 SamplesToWrite = 0;
					//uint32 CurrentSampleOffset;
					if (SUCCEEDED(GlobalSoundClient->GetCurrentPadding(&SoundPaddingSize))) {
						/*
						if (!IsAudioPlaying) {
							REFERENCE_TIME default_period;
							GlobalSoundClient->GetDevicePeriod(&default_period, NULL);
							// NOTE: Write Some Extra Samples in the first frame
							//WriteSampleOffset = SoundOutput.LatencySampleCount;
							//int Offset = (int) ((default_period / 10000000.0f) * SoundOutput.SamplesPerSecond);
							//WriteSampleOffset = SoundOutput.LatencySampleCount + Offset;
							WriteSampleOffset = 0;
						} else {
							WriteSampleOffset = (uint32) (SoundOutput.LatencySampleCount - LastPadding);
						}
						*/

						WriteSampleOffset = (uint32) (SoundOutput.LatencySampleCount - LastPadding);

						SamplesToWrite = BufferSizeInFrames - SoundPaddingSize;
						if (SamplesToWrite > SoundOutput.LatencySampleCount) {
							SamplesToWrite = SoundOutput.LatencySampleCount + WriteSampleOffset;
						} 
					}

					game_sound_buffer SoundBuffer = {};
					SoundBuffer.SampleCount = (int16) SamplesToWrite;
					SoundBuffer.Samples = Samples;
					SoundBuffer.SamplesPerSecond = (uint16) SoundOutput.SamplesPerSecond;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;

					game_keyboard_input Input = {};
					Win32GetKeyState(&Input);
					GameUpdateAndRender(&Buffer, &Input, &SoundBuffer, &Memory);

					Win32FillSoundBuffer(&SoundOutput, SamplesToWrite, &SoundBuffer);

					// NOTE: setting granularity just before sleep
					TIMECAPS PTC = {};
					timeGetDevCaps(&PTC, sizeof(PTC));
					UINT DesiredSchedulerMS = PTC.wPeriodMin; // 1 ms
					bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
						
					LARGE_INTEGER NowCounter = Win32GetWallClockTime();
					real32 FrameSecondsElasped = Win32GetSecondsElapsed(LastCounter, NowCounter);
					//real32 SecondsElapsed = NowCounter;

					if (FrameSecondsElasped < TargetSecondsPerFrame) {
						DWORD SleepMS;
						if (SleepIsGranular) {
							// NOTE: Sleep() always sleeps *slightly* more than the specified time
							//       so, subtract 1ms so that it spends that time in loop
							//       this is done just to get the exact value in the FPS mesurement.
							//       Does this matter?

							//DWORD SleepMS = (DWORD)(1000 * (TargetSecondsPerFrame - SecondsElapsed));
							SleepMS = (DWORD) (1000 * (TargetSecondsPerFrame - FrameSecondsElasped));
							if (SleepMS > 1) {
								SleepMS -= 1;
							}
							if (SleepMS > 0) {
								Sleep(SleepMS);
							}
							// NOTE: reset the 1ms sleep granularity
							//timeEndPeriod(DesiredSchedulerMS);
						}
						real32 Temp = Win32GetSecondsElapsed(NowCounter,
															 Win32GetWallClockTime());
						real32 TestSecondsElapsed = Win32GetSecondsElapsed(LastCounter,
																		   Win32GetWallClockTime()); 
						//Assert(TestSecondsElapsed < TargetSecondsPerFrame);
						if (TestSecondsElapsed > TargetSecondsPerFrame){
							OutputDebugStringA("Skipped!\n");
						}

						while(FrameSecondsElasped < TargetSecondsPerFrame) {
							FrameSecondsElasped = Win32GetSecondsElapsed(LastCounter,
																		 Win32GetWallClockTime()); 
						}
					} else {
						
					}

					LARGE_INTEGER CurrentCounter = Win32GetWallClockTime();

					real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, CurrentCounter);
					LastCounter = CurrentCounter;

					char OutputBuffer[256];
					GlobalSoundClient->GetCurrentPadding(&LastPadding);
					real32 TimeElapsed = Win32GetSecondsElapsed(StartTime,
																Win32GetWallClockTime());
					TimeElapsed *= 1000.0f;
					sprintf_s(OutputBuffer, 256, "Padding: %d TimeElapsed: %.2f\n",
							  WriteSampleOffset , TimeElapsed);
					OutputDebugStringA(OutputBuffer);
					

					if (!IsAudioPlaying) {
						IsAudioPlaying = true;
						GlobalSoundClient->Start();
					}

					HDC DeviceContext = GetDC(Window);
					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32UpdateWindow(DeviceContext,
									  Dimension.Width,
									  Dimension.Height,
									  &GlobalBackBuffer);
					ReleaseDC(Window, DeviceContext);


					uint64 CurrentCycleCounter = __rdtsc();
					LastCycleCounter = CurrentCycleCounter;

# if 0
					int64 CounterElapsed = CurrentCounter.QuadPart - LastCounter.QuadPart;
					uint64 CycleElapsed = CurrentCycleCounter - LastCycleCounter;
					//real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)CounterPerSecond.QuadPart;
					real32 FPS = (real32)CounterPerSecond.QuadPart / (real32)CounterElapsed;
					real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);
					char OutputBuffer[256];
					sprintf_s(OutputBuffer, 256, "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(OutputBuffer);
#endif
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
