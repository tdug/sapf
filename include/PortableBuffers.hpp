#pragma once

#ifndef SAPF_AUDIOTOOLBOX
#include <cstdint>
#include <vector>

#include "VM.hpp"

struct PortableBuffer {
	uint32_t numChannels;
	uint32_t size;
	void *data;

	PortableBuffer();
};

class PortableBuffers {
public:
	PortableBuffers(int inNumChannels);
	~PortableBuffers();

	uint32_t numChannels();
	void setNumChannels(size_t i, uint32_t numChannels);
	void setData(size_t i, void *data);
	void setSize(size_t i, uint32_t size);
	
	std::vector<PortableBuffer> buffers;
	std::vector<uint8_t> interleaved;
};
#endif // SAPF_AUDIOTOOLBOX
