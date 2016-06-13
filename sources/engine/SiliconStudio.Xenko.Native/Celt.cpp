// Copyright (c) 2014 Silicon Studio Corp. (http://siliconstudio.co.jp)
// This file is distributed under GPL v3. See LICENSE.md for details.

#include "../../../deps/NativePath/NativePath.h"
#include "../../../deps/NativePath/NativeDynamicLinking.h"

#define HAVE_STDINT_H
#include "../../../deps/Celt/include/opus_custom.h"
#include "../../../deps/OpenAL/AL/al.h"
#include "../../../deps/OpenAL/AL/alc.h"

extern "C" {
	namespace OpenAL
	{
		LPALCOPENDEVICE OpenDevice;
		LPALCCLOSEDEVICE CloseDevice;
		LPALCCREATECONTEXT CreateContext;
		LPALCDESTROYCONTEXT DestroyContext;
		LPALCMAKECONTEXTCURRENT MakeContextCurrent;
		
		LPALSOURCEPLAY SourcePlay;
		LPALSOURCEPAUSE SourcePause;
		LPALSOURCESTOP SourceStop;
		LPALSOURCEF SourceF;
		LPALDELETESOURCES DeleteSources;
		LPALDELETEBUFFERS DeleteBuffers;
		LPALGENSOURCES GenSources;
		LPALGENBUFFERS GenBuffers;
		LPALSOURCE3I Source3I;
		LPALSOURCEI SourceI;
		LPALBUFFERDATA BufferData;
		LPALSOURCEQUEUEBUFFERS SourceQueueBuffers;
		LPALSOURCEUNQUEUEBUFFERS SourceUnqueueBuffers;
		LPALGETSOURCEI GetSourceI;
		LPALSOURCEFV SourceFV;
		LPALLISTENERFV ListenerFV;

		void* OpenALLibrary = NULL;

		bool xnInitOpenAL()
		{
			if (OpenALLibrary) return true;

			OpenALLibrary = LoadDynamicLibrary("OpenAL32");
			if (!OpenALLibrary) OpenALLibrary = LoadDynamicLibrary("x64\\OpenAL32");
			if (!OpenALLibrary) OpenALLibrary = LoadDynamicLibrary("x86\\OpenAL32");
			if (!OpenALLibrary) OpenALLibrary = LoadDynamicLibrary("x64/OpenAL32");
			if (!OpenALLibrary) OpenALLibrary = LoadDynamicLibrary("x86/OpenAL32");
			if (!OpenALLibrary) OpenALLibrary = LoadDynamicLibrary(NULL);
			if (!OpenALLibrary) return false;

			OpenDevice = (LPALCOPENDEVICE)GetSymbolAddress(OpenALLibrary, "alcOpenDevice");
			if (!OpenDevice) return false;

			CloseDevice = (LPALCCLOSEDEVICE)GetSymbolAddress(OpenALLibrary, "alcCloseDevice");
			CreateContext = (LPALCCREATECONTEXT)GetSymbolAddress(OpenALLibrary, "alcCreateContext");
			DestroyContext = (LPALCDESTROYCONTEXT)GetSymbolAddress(OpenALLibrary, "alcDestroyContext");
			MakeContextCurrent = (LPALCMAKECONTEXTCURRENT)GetSymbolAddress(OpenALLibrary, "alcMakeContextCurrent");

			SourcePlay = (LPALSOURCEPLAY)GetSymbolAddress(OpenALLibrary, "alSourcePlay");
			SourcePause = (LPALSOURCEPAUSE)GetSymbolAddress(OpenALLibrary, "alSourcePause");
			SourceStop = (LPALSOURCESTOP)GetSymbolAddress(OpenALLibrary, "alSourceStop");
			SourceF = (LPALSOURCEF)GetSymbolAddress(OpenALLibrary, "alSourcef");
			DeleteSources = (LPALDELETESOURCES)GetSymbolAddress(OpenALLibrary, "alDeleteSources");
			DeleteBuffers = (LPALDELETEBUFFERS)GetSymbolAddress(OpenALLibrary, "alDeleteBuffers");
			GenSources = (LPALGENSOURCES)GetSymbolAddress(OpenALLibrary, "alGenSources");
			GenBuffers = (LPALGENBUFFERS)GetSymbolAddress(OpenALLibrary, "alGenBuffers");
			Source3I = (LPALSOURCE3I)GetSymbolAddress(OpenALLibrary, "alSource3i");
			SourceI = (LPALSOURCEI)GetSymbolAddress(OpenALLibrary, "alSourcei"); 
			BufferData = (LPALBUFFERDATA)GetSymbolAddress(OpenALLibrary, "alBufferData");
			SourceQueueBuffers = (LPALSOURCEQUEUEBUFFERS)GetSymbolAddress(OpenALLibrary, "alSourceQueueBuffers"); 
			SourceUnqueueBuffers = (LPALSOURCEUNQUEUEBUFFERS)GetSymbolAddress(OpenALLibrary, "alSourceUnqueueBuffers");
			GetSourceI = (LPALGETSOURCEI)GetSymbolAddress(OpenALLibrary, "alGetSourcei");
			SourceFV = (LPALSOURCEFV)GetSymbolAddress(OpenALLibrary, "alSourcefv");
			ListenerFV = (LPALLISTENERFV)GetSymbolAddress(OpenALLibrary, "alListenerfv");

			return true;
		}

		struct xnAudioDevice
		{
			ALCdevice* device;
			ALCcontext* context;
		};

		xnAudioDevice* xnAudioCreate(const char* deviceName)
		{
			auto o = new xnAudioDevice;
			o->device = OpenDevice(deviceName);
			if(!o->device)
			{
				delete o;
				return NULL;
			}
			o->context = CreateContext(o->device, NULL);
			MakeContextCurrent(o->context);
			return o;
		}

		void xnAudioDestroy(xnAudioDevice* device)
		{
			MakeContextCurrent(NULL);
			DestroyContext(device->context);
			CloseDevice(device->device);
			delete device;
		}

		uint32_t xnAudioSourceCreate()
		{
			ALuint source;
			GenSources(1, &source);
			SourceF(source, AL_REFERENCE_DISTANCE, 1.0f);
			return source;
		}

		void xnAudioSourceDestroy(uint32_t source)
		{
			DeleteSources(1, &source);
		}

		void xnAudioSourceSetPan(uint32_t source, float pan)
		{
			//make sure we are able to pan
			SourceI(source, AL_SOURCE_RELATIVE, AL_TRUE);

			auto clampedPan = pan > 1.0f ? 1.0f : pan < -1.0f ? -1.0f : pan;
			ALfloat alpan[3];
			alpan[0] = clampedPan; // from -1 (left) to +1 (right) 
			alpan[1] = sqrt(1.0f - clampedPan*clampedPan);
			alpan[2] = 0.0f;
			SourceFV(source, AL_POSITION, alpan);
		}

		void xnAudioSourceSetLooping(uint32_t source, bool looping)
		{
			SourceI(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
		}

		void xnAudioSourceSetGain(uint32_t source, float gain)
		{
			SourceF(source, AL_GAIN, gain);
		}

		void xnAudioSourceSetPitch(uint32_t source, float pitch)
		{
			SourceF(source, AL_PITCH, pitch);
		}

		uint32_t xnAudioBufferCreate()
		{
			ALuint buffer;
			GenBuffers(1, &buffer);
			return buffer;
		}

		void xnAudioBufferDestroy(uint32_t buffer)
		{
			DeleteBuffers(1, &buffer);
		}

		void xnAudioBufferFill(uint32_t buffer, short* pcm, int bufferSize, int sampleRate, bool mono)
		{
			BufferData(buffer, mono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, bufferSize, sampleRate);
		}

		void xnAudioSourceSetBuffer(uint32_t source, uint32_t buffer)
		{
			SourceI(source, AL_BUFFER, buffer);
		}

		void xnAudioSourceQueueBuffer(uint32_t source, uint32_t buffer, short* pcm, int bufferSize, int sampleRate, bool mono)
		{
			BufferData(buffer, mono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, bufferSize, sampleRate);
			SourceQueueBuffers(source, 1, &buffer);
		}

		uint32_t xnAudioSourceGetFreeBuffer(uint32_t source)
		{
			ALint processed = 0;
			GetSourceI(source, AL_BUFFERS_PROCESSED, &processed);
			if(processed > 0)
			{
				ALuint buffer;
				SourceUnqueueBuffers(source, 1, &buffer);
				return buffer;
			}
			return 0;
		}

		void xnAudioSourcePlay(uint32_t source)
		{
			SourcePlay(source);
		}

		void xnAudioSourcePause(uint32_t source)
		{
			SourcePause(source);
		}

		void xnAudioSourceStop(uint32_t source)
		{
			SourceStop(source);
		}

		void xnAudioListenerPush3D(float* pos, float* rot, float* vel)
		{
			if (pos) ListenerFV(AL_POSITION, pos);
			if (rot) ListenerFV(AL_ORIENTATION, rot);
			if (vel) ListenerFV(AL_VELOCITY, vel);
		}

		void xnAudioSourcePush3D(uint32_t source, float* pos, float* rot, float* vel)
		{
			//make sure we are able to 3D
			SourceI(source, AL_SOURCE_RELATIVE, AL_FALSE);

			if (pos) SourceFV(source, AL_POSITION, pos);
			if (rot) SourceFV(source, AL_ORIENTATION, rot);
			if (vel) SourceFV(source, AL_VELOCITY, vel);
		}

		bool xnAudioSourceIsPlaying(uint32_t source)
		{
			ALint value;
			GetSourceI(source, AL_SOURCE_STATE, &value);
			return value == AL_PLAYING;
		}
	}

	class XenkoCelt
	{
	public:
		XenkoCelt(int sampleRate, int bufferSize, int channels, bool decoderOnly): mode_(nullptr), decoder_(nullptr), encoder_(nullptr), sample_rate_(sampleRate), buffer_size_(bufferSize), channels_(channels), decoder_only_(decoderOnly)
		{
		}

		~XenkoCelt()
		{
			if (encoder_) opus_custom_encoder_destroy(encoder_);
			encoder_ = nullptr;
			if (decoder_) opus_custom_decoder_destroy(decoder_);
			decoder_ = nullptr;
			if (mode_) opus_custom_mode_destroy(mode_);
			mode_ = nullptr;
		}

		bool Init()
		{
			mode_ = opus_custom_mode_create(sample_rate_, buffer_size_, nullptr);
			if (!mode_) return false;

			decoder_ = opus_custom_decoder_create(mode_, channels_, nullptr);
			if (!decoder_) return false;

			if(!decoder_only_)
			{
				encoder_ = opus_custom_encoder_create(mode_, channels_, nullptr);
				if (!encoder_) return false;
			}

			return true;
		}

		OpusCustomEncoder* GetEncoder() const
		{
			return encoder_;
		}

		OpusCustomDecoder* GetDecoder() const
		{
			return decoder_;
		}

	private:
		OpusCustomMode* mode_;
		OpusCustomDecoder* decoder_;
		OpusCustomEncoder* encoder_;
		int sample_rate_;
		int buffer_size_;
		int channels_;
		bool decoder_only_;
	};

	void* xnCeltCreate(int sampleRate, int bufferSize, int channels, bool decoderOnly)
	{
		auto celt = new XenkoCelt(sampleRate, bufferSize, channels, decoderOnly);
		if(!celt->Init())
		{
			delete celt;
			return nullptr;
		}
		return celt;
	}

	void xnCeltDestroy(XenkoCelt* celt)
	{
		delete celt;
	}

	int xnCeltEncodeFloat(XenkoCelt* celt, float* inputSamples, int numberOfInputSamples, uint8_t* outputBuffer, int maxOutputSize)
	{
		return opus_custom_encode_float(celt->GetEncoder(), inputSamples, numberOfInputSamples, outputBuffer, maxOutputSize);
	}

	int xnCeltDecodeFloat(XenkoCelt* celt, uint8_t* inputBuffer, int inputBufferSize, float* outputBuffer, int numberOfOutputSamples)
	{
		return opus_custom_decode_float(celt->GetDecoder(), inputBuffer, inputBufferSize, outputBuffer, numberOfOutputSamples);
	}

	int xnCeltEncodeShort(XenkoCelt* celt, int16_t* inputSamples, int numberOfInputSamples, uint8_t* outputBuffer, int maxOutputSize)
	{
		return opus_custom_encode(celt->GetEncoder(), inputSamples, numberOfInputSamples, outputBuffer, maxOutputSize);
	}

	int xnCeltDecodeShort(XenkoCelt* celt, uint8_t* inputBuffer, int inputBufferSize, int16_t* outputBuffer, int numberOfOutputSamples)
	{
		return opus_custom_decode(celt->GetDecoder(), inputBuffer, inputBufferSize, outputBuffer, numberOfOutputSamples);
	}

}
