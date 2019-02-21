#ifndef __AUDIO_CAPTURE_API__
#define __AUDIO_CAPTURE_API__

#ifdef __cplusplus
extern "C"{
#endif


/* Acoustic Echo Cancellation */

/**
	\brief initialize Aec to get an instance.
    \param [in] frameLength   - Length of one frame for processing, must be LESS or EQUAL to 256 at 16kHz and 512 at other sampling rates.
	\param [in] filterLength  - Length of the filter, must be 2^n, equal or greater than 2048;
	\param [in] channelN      - Number of the audio signal, must be set to 1 for this version..
	\param [in] sRate         - sample Rate. support all.	
	\param [in] sysDelay      - set it to 0 unless you know the amount of system latency that can be compensated.
	\param [in] micN          - number of mic.
	\param [in] spkN          - number of speaker.
	\param [in] multithread   - Flag of turning on multi-threads whiling computing, must be false for this version.
	                           This is a performance parameter and the behavior
                               depends on specific platform and processor.
	\return The initialized object of Aec or null pointer that means failed.
*/
void * aecInit(int frameLength, int filterLength, int sRate, int sysDelay, int micN, int spkN, bool multithread);

/**
	\brief set audio aec object.
	\param [in] obj            - aec instance pointer.
	\param [in] enableRes      - Flag of enable residual echo suppression.
	\param [in] resLevel       - 0~10, suppress more residual echo if you set this value high. The downside is possible near end voice distortion.
	\param [in] enableNS       - Flag of enable noise suppression.
	\param [in] nsDB           - maximum noise suppression in DB. -6dB ~ -20dB.
	\param [in] enableSpkClip  - Flag of speaker clipping control, set it false unless you know the speaker nolinear clip character.
	\param [in] spkClipThd     - 0~1.
	\param [in] maxCoupling    - max coupling factor between spk to mic. 0.5~20
	\param [in] adaptSetp      - adaptive filter step size. 0.1~1.0
*/
void aecSet(void *obj, bool enableRes, float resLevel, bool enableNS, float nsDB, bool enableSpkClip, float spkClipThd, float maxCoupling, float adaptStep);

/**
	\brief process by Aec object.
	\param [in] obj          - aec instance pointer.
	\param [in] audioSpk     - normalized speaker input audio pcm sample sequence. (value: -1~1)
	\param [in/out] audioMic - normalized mic input audio pcm sample sequence. (value: -1~1)
							   also normalized output audio pcm sample sequence.  (value: -1~1)
	                           pcm sample sequence length is 256.
*/
void aecProcess(void *obj, const float *audioSpk, float *audioMic);

/**
	\brief release Aec instance.
	\param [in] obj - The Aec instance to be released.
*/
void aecRelease(void *obj);


#ifdef __cplusplus				
};
#endif

#endif /*__AUDIO_CAPTURE_API__*/
