#ifndef SAPF_AUDIOTOOLBOX
#include "PortableBuffers.hpp"

PortableBuffer::PortableBuffer()
	: numChannels(0), size(0), data(nullptr)
{}

PortableBuffers::PortableBuffers(int inNumChannels)
	: buffers(inNumChannels)
{}

PortableBuffers::~PortableBuffers() {}

uint32_t PortableBuffers::numChannels() {
	return this->buffers.size();
}

void PortableBuffers::setNumChannels(size_t i, uint32_t numChannels) {
	this->buffers[i].numChannels = numChannels;
}

void PortableBuffers::setData(size_t i, void *data) {
	this->buffers[i].data = data;
}

void PortableBuffers::setSize(size_t i, uint32_t size) {
	this->buffers[i].size = size;
	this->interleaved.resize(size * this->numChannels());
}
#endif // SAPF_AUDIOTOOLBOX
