#define Kilobytes(Value) (1024*Value)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define GetKey(Key) ((1 << 15) & Key)

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
	uint32 SamplesPerSecond;
	uint32 BytesPerSample;
	uint32 BufferSize;
	uint32 LatencySampleCount;
	real32 tSine;
	int WavePeriod;
	uint32 RunningSampleIndex;
};

