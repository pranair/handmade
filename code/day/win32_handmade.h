
struct win32_offscreen_buffer {
	void *Memory;
	BITMAPINFO Info;
	int BytesPerPixel;
	int Width;
	int Height;
};

struct win32_window_dimension {
	int Width;
	int Height;
};

struct win32_sound_output {
	int32 SamplesPerSecond;
	int32 BytesPerSample;
	int32 BufferSize;
	int32 LatencySampleCount;
	real32 tSine;
	int WavePeriod;
	uint32 RunningSampleIndex;
};

