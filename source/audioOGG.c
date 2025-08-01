// ███████████████████████████████████████████████
// █▄─▄─▀█─▄▄─█▄─▄▄▀█▄─▄▄─█▄─▄▄▀█▄▄▄ █▄─▄▄▀█─▄▄▄▄█
// ██─▄─▀█─██─██─▄─▄██─▄█▀██─██─██▄▄ ██─██─█▄▄▄▄─█
// █▄▄▄▄██▄▄▄▄█▄▄█▄▄█▄▄▄▄▄█▄▄▄▄██▄▄▄▄█▄▄▄▄██▄▄▄▄▄█

// ▄▀█ █ █ █▀▄ █ █▀█   █▀▀
// █▀█ █▄█ █▄▀ █ █▄█ ▄ █▄▄

// █ █ █▀█ █▀█ █▄▄ █ █▀   ▄▀█ █ █ █▀▄ █ █▀█   █▀ █▄█ █▀ ▀█▀ █▀▀ █▀▄▀█
// ▀▄▀ █▄█ █▀▄ █▄█ █ ▄█   █▀█ █▄█ █▄▀ █ █▄█   ▄█  █  ▄█  █  ██▄ █ ▀ █

//   ╔════════════════════════════════════════════════╗
// ══╣                    INCLUDES                    ╠══
//   ╚════════════════════════════════════════════════╝
#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>


//   ╔════════════════════════════════════════════════╗
// ══╣                  DEFINITIONS                   ╠══
//   ╚════════════════════════════════════════════════╝
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0])) /** @brief Macro to get the size of an array */
#define MAX_STREAMS 8 /** @brief Maximum number of audio streams that can be played simultaneously */
#define THREAD_STACK_SZ (32 * 1024) /** @brief Stack size for audio threads */
#define THREAD_AFFINITY -1 /** @brief Thread affinity for audio threads */


//   ╔════════════════════════════════════════════════╗
// ══╣             AUDIO STREAM STRUCTURE             ╠══
//   ╚════════════════════════════════════════════════╝
typedef struct {
    int id; /** @brief Unique identifier for the audio stream */
    bool active; /** @brief Flag indicating if the audio stream is active */
    bool loop; /** @brief Flag indicating if the audio stream should loop */
    bool quit; /** @brief Flag indicating if the audio stream should quit */
    int channel; /** @brief Audio channel for the stream */

    OggVorbis_File vorbisFile; /** @brief Vorbis file handle */
    FILE *fileHandle; /** @brief File handle for the audio stream */

    ndspWaveBuf waveBufs[3]; /** @brief NDSP wave buffers for the audio stream */
    int16_t *audioBuffer; /** @brief Pointer to the audio buffer for the stream */

    LightEvent event; /** @brief Event for signaling audio thread */
    Thread thread; /** @brief Thread for audio playback */
} AudioStream;

static AudioStream streams[MAX_STREAMS];
static int nextId = 1;


/**
 * @fn static const char *vorbisStrError(int error)
 * @brief Converts a vorbis error code to a string.
 * @since rev12 (v0.0.1a)
 * @param error The error code to convert.
 * @returns A string representation of the error code.
 */
static const char *vorbisStrError(int error) {
    switch(error) {
        case OV_FALSE: return "OV_FALSE";
        case OV_HOLE: return "OV_HOLE";
        case OV_EREAD: return "OV_EREAD";
        case OV_EFAULT: return "OV_EFAULT";
        case OV_EIMPL: return "OV_EIMPL";
        case OV_EINVAL: return "OV_EINVAL";
        case OV_ENOTVORBIS: return "OV_ENOTVORBIS";
        case OV_EBADHEADER: return "OV_EBADHEADER";
        case OV_EVERSION: return "OV_EVERSION";
        case OV_EBADPACKET: return "OV_EBADPACKET";
        case OV_EBADLINK: return "OV_EBADLINK";
        case OV_ENOSEEK: return "OV_ENOSEEK";
        default: return "Unknown";
    }
}


/**
 * @fn static bool initStreamBuffers(AudioStream *s)
 * @brief Initializes the stream buffers for the given AudioStream.
 * @since rev12 (v0.0.1a)
 * @param[in] s The AudioStream to initialize the stream buffers for.
 * @returns true if the initialization was successful, false otherwise.
 */
static bool initStreamBuffers(AudioStream *s) {
    vorbis_info *vi = ov_info(&s->vorbisFile, -1);

    ndspChnReset(s->channel);
    ndspChnSetInterp(s->channel, vi->rate);
    ndspChnSetRate(s->channel, vi->rate);
    ndspChnSetFormat(s->channel, vi->channels == 1 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);

    const size_t samplesPerBuf = vi->rate * 120 / 1000; // 120ms buffer
    const size_t channelsPerSample = vi->channels;
    const size_t waveBufSize = samplesPerBuf * channelsPerSample * sizeof(s16);
    const size_t bufferSize = waveBufSize * ARRAY_SIZE(s->waveBufs);

    s->audioBuffer = (int16_t *)linearAlloc(bufferSize);
    if (!s->audioBuffer) return false;

    memset(s->waveBufs, 0, sizeof(s->waveBufs));
    int16_t *buf = s->audioBuffer;
    for (size_t i = 0; i < ARRAY_SIZE(s->waveBufs); ++i) {
        s->waveBufs[i].data_vaddr = buf;
        s->waveBufs[i].nsamples = waveBufSize / sizeof(buf[0]);
        s->waveBufs[i].status = NDSP_WBUF_DONE;
        buf += waveBufSize / sizeof(buf[0]);
    }

    return true;
}

/**
 * @fn static bool fillBuffer(AudioStream *s, ndspWaveBuf *waveBuf)
 * @brief Decodes audio samples from a Vorbis file and fills the provided NDSP wave buffer.
 * @since rev12 (v0.0.1a)
 * @param[in] s The AudioStream structure containing the Vorbis file and channel information.
 * @param[in] waveBuf The NDSP wave buffer to be filled with decoded audio samples.
 * @returns true if samples were successfully read and the buffer was filled, false if no samples were read.
 */
static bool fillBuffer(AudioStream *s, ndspWaveBuf *waveBuf) {
    int totalBytes = 0;
    while (totalBytes < waveBuf->nsamples * sizeof(s16)) {
        int16_t *buffer = waveBuf->data_pcm16 + (totalBytes / sizeof(s16));
        const size_t bufferSize = (waveBuf->nsamples * sizeof(s16) - totalBytes);
        int bytesRead = ov_read(&s->vorbisFile, (char *)buffer, bufferSize, NULL);
        if (bytesRead <= 0) {
            if (bytesRead == 0) break;
            printf("ov_read error: %s\n", vorbisStrError(bytesRead));
            break;
        }
        totalBytes += bytesRead;
    }

    if (totalBytes == 0) return false;

    waveBuf->nsamples = totalBytes / sizeof(s16);
    ndspChnWaveBufAdd(s->channel, waveBuf);
    DSP_FlushDataCache(waveBuf->data_pcm16, totalBytes);
    return true;
}

/**
 * @fn static void audioThread(void *arg)
 * @brief Thread function for playing back audio.
 * @since rev12 (v0.0.1a)
 * @param[in] arg Pointer to the AudioStream structure containing the audio data and channel information.
 */
static void audioThread(void *arg) {
    AudioStream *s = (AudioStream *)arg;

    while (!s->quit) {
        for (size_t i = 0; i < ARRAY_SIZE(s->waveBufs); ++i) {
            if (s->waveBufs[i].status != NDSP_WBUF_DONE) continue;
            if (!fillBuffer(s, &s->waveBufs[i])) {
                if (s->loop) {
                    ov_raw_seek(&s->vorbisFile, 0);
                    i--;
                    continue;
                }
                s->quit = true;
                break;
            }
        }
        LightEvent_Wait(&s->event);
    }
    s->active = false;
}

/**
 * @fn static void ndspCallback(void *unused)
 * @brief NDSP audio frame callback
 * @since rev12 (v0.0.1a)
 * @details This signals the audio thread to fill the wave buffer again once NDSP has played a sound frame, meaning that there should be one or more available waveBufs to fill with more data.
 */
static void audioNdspCallback(void *unused) {
    (void)unused;
    for (int i = 0; i < MAX_STREAMS; i++) {
        if (streams[i].active && !streams[i].quit) {
            LightEvent_Signal(&streams[i].event);
        }
    }
}   

/**
 * @fn void audioInitSystem();
 * @brief Initializes the audio system.
 * @since rev12 (v0.0.1a)
 * @note This function must be called before any other audio functions.
 */
void audioInitSystem(void) {
    memset(streams, 0, sizeof(streams));
    ndspInit();
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspSetCallback(audioNdspCallback, NULL);
}

/**
 * @fn int audioPlay(const char *path, bool loop);
 * @brief Plays an vorbis (OGG) audio file from the specified path.
 * @since rev12 (v0.0.1a)
 * @param path The path to the audio file to play.
 * @param loop Whether to loop the audio file.
 * @returns Audio ID if successful, or -1 if an error occurred.
 * @note This function can be called after initializing the audio system.
 * @note The 3DS uses romfs for audio files, so the path should be in the format "romfs:/path/to/audio.ogg".
 */
int audioPlay(const char *path, bool loop) {
    int slot = -1;
    for (int i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].active) { slot = i; break; }
    }
    if (slot == -1) return -1;

    AudioStream *s = &streams[slot];
    memset(s, 0, sizeof(*s));
    s->id = nextId++;
    s->loop = loop;
    s->active = true;
    s->quit = false;
    s->channel = slot;

    s->fileHandle = fopen(path, "rb");
    if (!s->fileHandle) { s->active = false; return -1; }

    int err = ov_open(s->fileHandle, &s->vorbisFile, NULL, 0);
    if (err) {
        printf("ov_open error: %s\n", vorbisStrError(err));
        fclose(s->fileHandle);
        s->active = false;
        return -1;
    }

    if (!initStreamBuffers(s)) {
        ov_clear(&s->vorbisFile);
        fclose(s->fileHandle);
        s->active = false;
        return -1;
    }

    LightEvent_Init(&s->event, RESET_ONESHOT);

    s32 priority;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority = (priority > 0x18) ? priority - 1 : 0x18;

    s->thread = threadCreate(audioThread, s, THREAD_STACK_SZ, priority, THREAD_AFFINITY, false);
    if (!s->thread) {
        linearFree(s->audioBuffer);
        ov_clear(&s->vorbisFile);
        fclose(s->fileHandle);
        s->active = false;
        return -1;
    }

    return s->id;
}

/**
 * @fn void audioStop(int id);
 * @brief Stops the audio playback for the specified audio ID.
 * @since rev12 (v0.0.1a)
 * @param id The audio ID of the playback to stop.
 */
void audioStop(int id) {
    for (int i = 0; i < MAX_STREAMS; i++) {
        AudioStream *s = &streams[i];
        if (s->active && s->id == id) {
            s->quit = true;
            LightEvent_Signal(&s->event);
            threadJoin(s->thread, UINT64_MAX);
            threadFree(s->thread);

            ndspChnReset(s->channel);
            linearFree(s->audioBuffer);
            ov_clear(&s->vorbisFile);
            fclose(s->fileHandle);

            s->active = false;
            return;
        }
    }
}

/**
 * @fn void audioStopAll(void);
 * @brief Stops all audio playback.
 * @since rev12 (v0.0.1a)
 * @note This function stops all currently playing audio, regardless of ID.
 * @note Reccommended to use this function when exiting the program.
 */
void audioStopAll(void) {
    for (int i = 0; i < MAX_STREAMS; i++) {
        if (streams[i].active) {
            audioStop(streams[i].id);
        }
    }
}

/**
 * @fn void audioStopAll(void);
 * @brief Stops all audio playback.
 * @since rev12 (v0.0.1a)
 * @note This function stops all currently playing audio, regardless of ID.
 */
void audioExitSystem(void) {
    audioStopAll();
    ndspExit();
}