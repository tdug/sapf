#pragma once

#ifdef SAPF_AUDIOTOOLBOX
#include <AudioToolbox/AudioToolbox.h>

class AudioToolboxBuffers {
public:
	AudioToolboxBuffers(int inNumChannels);
	~AudioToolboxBuffers();

	uint32_t numChannels();
	void setNumChannels(size_t i, uint32_t numChannels);
	void setData(size_t i, void *data);
	void setSize(size_t i, uint32_t size);
	
	AudioBufferList *abl;
};
#endif // SAPF_AUDIOTOOLBOX
