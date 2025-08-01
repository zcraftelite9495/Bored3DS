#ifndef headerAudioOGG
#define headerAudioOGG

#include <stdbool.h>

/**
 * @fn void audioInitSystem();
 * @brief Initializes the audio system.
 * @since rev12 (v0.0.1a)
 * @note This function must be called before any other audio functions.
 */
void audioInitSystem(void);

/**
 * @fn void audioExitSystem();
 * @brief Exits the audio system.
 * @since rev12 (v0.0.1a)
 * @note This function must be called before the program exits.
 */
void audioExitSystem(void);

/**
 * @fn int audioPlay(const char *path, bool loop);
 * @brief Plays an waveform (WAV) audio file from the specified path.
 * @since rev12 (v0.0.1a)
 * @param path The path to the audio file to play.
 * @param loop Whether to loop the audio file.
 * @returns Audio ID if successful, or -1 if an error occurred.
 * @note This function can be called after initializing the audio system.
 * @note The 3DS uses romfs for audio files, so the path should be in the format "romfs:/path/to/audio.wav".
 */
int audioPlay(const char *path, bool loop);

/**
 * @fn void audioStop(int id);
 * @brief Stops the audio playback for the specified audio ID.
 * @since rev12 (v0.0.1a)
 * @param id The audio ID of the playback to stop.
 */
void audioStop(int id);

/**
 * @fn void audioStopAll(void);
 * @brief Stops all audio playback.
 * @since rev12 (v0.0.1a)
 * @note This function stops all currently playing audio, regardless of ID.
 * @note Reccommended to use this function when exiting the program.
 */
void audioStopAll(void);

#endif // headerAudioOGG