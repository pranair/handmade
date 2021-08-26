#include "handmade.h"

void
GameFillSoundBuffer(game_sound_buffer *SoundBuffer, game_state *State)
{
	int16 *SampleOut = (int16 *)SoundBuffer->Samples;
	//int ToneHz = 256;
	int16 WavePeriod = (int16)(SoundBuffer->SamplesPerSecond / State->ToneHz);
	int16 ToneVolume = 1000;
	//if (tSine > 10000) {
	//tSine = 0;
	//}

# if 0
	for(int i = 0; i < SoundBuffer->SampleCount; i++) {
		real32 SineValue = sinf(State->tSine);
		int16 Value = (int16)(SineValue * ToneVolume);
		*SampleOut++ = Value;
		*SampleOut++ = Value;
		State->tSine += (real32) (2.0f * Pi32 * 1.0f / (real32)WavePeriod);
	}
#endif
}

void RenderTinyPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
	int Pitch = Buffer->Width * Buffer->BytesPerPixel;
	for(int X = PlayerX; X < PlayerX + 10; X++) {
		uint8 *Pixel = ((uint8*)Buffer->Memory +
						 X * Buffer->BytesPerPixel +
						 PlayerY * Pitch);
		
		for (int Y = PlayerY; Y < PlayerY + 10; Y++) {
			*(uint32 *)Pixel= 0xFFFFFFFF;
			Pixel += Pitch;
		}
	}

}

void
RenderWeirdGradient(game_offscreen_buffer *Buffer, game_state *State)
{
	uint8 *Row = (uint8 *)Buffer->Memory;

	for(int Y = 0; Y < Buffer->Height; Y++) {
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0; X < Buffer->Width; X++) {
			uint8 Blue = (uint8) (X + State->XOffset);
			uint8 Green = (uint8) (Y + State->YOffset);
			// 0xXXRRGGBB
			*Pixel++ = (Green << 8) | Blue;
		}

		Row = Row + Buffer->Width * Buffer->BytesPerPixel;
	}
}

/*
void
GameUpdateAndRender (game_offscreen_buffer *Buffer, game_keyboard_input *KBInput,
					 game_sound_buffer *SoundBuffer, game_memory *Memory) {
*/
GAME_UPDATE_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermenantStorageSize);
	local_persist game_state *State = (game_state *) Memory->PermenantStorage;

	
	/*
	debug_read_entire_file File = Memory->DEBUGPlatformReadEntireFile("C:\\Users\\Pratyush Nair\\Documents\\code\\boring\\build\\test.txt");
	if (File.Content) {
		Memory->DEBUGPlatformWriteEntireFile("C:\\Users\\Pratyush Nair\\Documents\\code\\boring\\build\\out.txt",
											 File.ContentSize, File.Content);
		Memory->DEBUGPlatformFreeFileMemory(File.Content);
	} else {
		
	}
	*/
	static int PlayerX = Buffer->Height/2;
	static int PlayerY = Buffer->Width/2;
	
	
	if (!Memory->IsIntialized) {
		State->ToneHz = 256;
		State->tSine = 0;
		Memory->IsIntialized = true;
	}

	if (KBInput->IsDown) {
		PlayerY += 20;
		//PlayerY += 20;
		State->YOffset -= 20;
	}

	if (KBInput->IsUp) {
		PlayerY -= 20;
		//PlayerY -= 20;
		State->YOffset += 20;
	}

	if (KBInput->IsLeft) {
		PlayerX += 20;
	}

	if (KBInput->IsRight) {
		PlayerX -= 20;
	}

	State->ToneHz = (int)(((real32)State->YOffset / 1000) * 256) + 250;

	GameFillSoundBuffer(SoundBuffer, State);
	RenderWeirdGradient(Buffer, State);
	RenderTinyPlayer(Buffer, PlayerX, PlayerY);
}
