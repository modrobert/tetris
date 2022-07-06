#define SOUND_LINE_DROP "drop.wav"
#define SOUND_LINE_CLEAR "clear.wav"
#define SOUND_HISCORE "hiscore.wav"

void drop_audio_callback(void *userdata, Uint8 *stream, int len);
void clear_audio_callback(void *userdata, Uint8 *stream, int len);
void hiscore_audio_callback(void *userdata, Uint8 *stream, int len);
void play_drop_sound();
void play_clear_sound();
void play_hiscore_sound();

// Global pointer to the audio buffers to be played.
static Uint8 *audio_drop_pos, *audio_clear_pos, *audio_hiscore_pos;
// Remaining length of the sample we have to play.
static Uint32 audio_drop_len, audio_clear_len, audio_hiscore_len;

// Audio buffers etc.
static Uint32 wav_drop_length, wav_clear_length, wav_hiscore_length;
static Uint8 *wav_drop_buffer, *wav_clear_buffer, *wav_hiscore_buffer;
static SDL_AudioSpec wav_drop_spec, wav_clear_spec, wav_hiscore_spec;


void drop_audio_callback(void *userdata, Uint8 *stream, int len)
{
    if (audio_drop_len ==0)
        return;

    len = ( len > audio_drop_len ? audio_drop_len : len );
    SDL_MixAudio(stream, audio_drop_pos, len, 64);
    audio_drop_pos += len;
    audio_drop_len -= len;
}

void clear_audio_callback(void *userdata, Uint8 *stream, int len)
{
    if (audio_clear_len == 0)
        return;

    len = ( len > audio_clear_len ? audio_clear_len : len );
    SDL_MixAudio(stream, audio_clear_pos, len, 84);
    audio_clear_pos += len;
    audio_clear_len -= len;
}

void hiscore_audio_callback(void *userdata, Uint8 *stream, int len)
{
    if (audio_hiscore_len == 0)
        return;

    len = ( len > audio_hiscore_len ? audio_hiscore_len : len );
    SDL_MixAudio(stream, audio_hiscore_pos, len, SDL_MIX_MAXVOLUME);
    audio_hiscore_pos += len;
    audio_hiscore_len -= len;
}

void play_drop_sound()
{
    // Set global variables.
    audio_drop_pos = wav_drop_buffer;
    audio_drop_len = wav_drop_length;

    // Open the audio device.
    if ( SDL_OpenAudio(&wav_drop_spec, NULL) < 0 ){
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        exit(-1);
    }
	// Start playing.
	SDL_PauseAudio(0);
}

void play_clear_sound()
{
    // Set global variables.
    audio_clear_pos = wav_clear_buffer;
    audio_clear_len = wav_clear_length;

    // Open the audio device.
    if ( SDL_OpenAudio(&wav_clear_spec, NULL) < 0 ){
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        exit(-1);
    }
	// Start playing.
	SDL_PauseAudio(0);
}

void play_hiscore_sound()
{
    // Set global variables.
    audio_hiscore_pos = wav_hiscore_buffer;
    audio_hiscore_len = wav_hiscore_length;

    // Open the audio device.
    if ( SDL_OpenAudio(&wav_hiscore_spec, NULL) < 0 ){
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        exit(-1);
    }
	// Start playing.
	SDL_PauseAudio(0);
}

