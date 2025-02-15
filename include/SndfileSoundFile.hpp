#pragma once

#ifndef SAPF_AUDIOTOOLBOX
#include "PortableBuffers.hpp"

#include <memory>
#include <vector>
#include <sndfile.h>

class SndfileSoundFile {
public:
	SndfileSoundFile(SNDFILE *inSndfile, int inNumChannels);
	~SndfileSoundFile();

	uint32_t numChannels();
	int pull(uint32_t *framesRead, PortableBuffers& buffers);
	
	SNDFILE *mSndfile;
	std::vector<double> mBufInterleaved;
	int mNumChannels;

	static std::unique_ptr<SndfileSoundFile> open(const char *path);
	static std::unique_ptr<SndfileSoundFile> create(const char *path, int numChannels, double threadSampleRate, double fileSampleRate, bool interleaved);
};
#endif // SAPF_AUDIOTOOLBOX
