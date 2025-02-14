//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "SoundFiles.hpp"
#include <valarray>

extern char gSessionTime[256];

#ifdef SAPF_AUDIOTOOLBOX
class AudioToolboxBuffers {
public:
	AudioToolboxBuffers(int inNumChannels) {
		this->abl = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + (inNumChannels - 1) * sizeof(AudioBuffer));
		this->abl->mNumChannels = inNumChannels;
	}

	~AudioToolboxBuffers() {
		free(this->abl);
	}

	uint32_t numChannels() {
		return this->abl->mNumChannels;
	}

	void setData(size_t i, Z *data, uint32_t blockSize) {
		AudioBuffer buf = this->abl[i];
		buf.mNumberChannels = 1;
		buf.mData = data;
		buf.mDataByteSize = blockSize * sizeof(Z);
	}
	
	AudioBufferList *abl;
};

typedef AudioToolboxBuffers AudioBuffers;

class AudioToolboxSFReaderBackend {
public:
	AudioToolboxSFReaderBackend(ExtAudioFileRef inXAF, uint32_t inNumChannels)
		: mXAF(inXAF), mNumChannels(inNumChannels)
	{}

	~AudioToolboxSFReaderBackend() {
		ExtAudioFileDispose(this->mXAF);
	}

	uint32_t numChannels() {
		return this->mNumChannels;
	}

	int pull(uint32_t *framesRead, AudioBuffers& buffers) {
		return ExtAudioFileRead(this->mXAF, framesRead, buffers.abl);
	}
	
	ExtAudioFileRef mXAF;
	uint32_t mNumChannels;

	static AudioToolboxSFReaderBackend *openForReading(const char *path) {
		CFStringRef cfpath = CFStringCreateWithFileSystemRepresentation(0, path);
		if (!cfpath) {
			post("failed to create path\n");
			return nullptr;
		}
		CFReleaser cfpathReleaser(cfpath);
	
		CFURLRef url = CFURLCreateWithFileSystemPath(0, cfpath, kCFURLPOSIXPathStyle, false);
		if (!url) {
			post("failed to create url\n");
			return nullptr;
		}
		CFReleaser urlReleaser(url);
	
		ExtAudioFileRef xaf;
		OSStatus err = ExtAudioFileOpenURL(url, &xaf);

		cfpathReleaser.release();
		urlReleaser.release();
        
		if (err) {
			post("failed to open file %d\n", (int)err);
			return nullptr;
		}

		AudioStreamBasicDescription fileFormat;
	
		UInt32 propSize = sizeof(fileFormat);
		err = ExtAudioFileGetProperty(xaf, kExtAudioFileProperty_FileDataFormat, &propSize, &fileFormat);
	
		int numChannels = fileFormat.mChannelsPerFrame;

		AudioStreamBasicDescription clientFormat = {
			th.rate.sampleRate,
			kAudioFormatLinearPCM,
			kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved,
			static_cast<UInt32>(sizeof(double)),
			1,
			static_cast<UInt32>(sizeof(double)),
			static_cast<UInt32>(numChannels),
			64,
			0
		};
	
		err = ExtAudioFileSetProperty(xaf, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
		if (err) {
			post("failed to set client data format\n");
			ExtAudioFileDispose(xaf);
			return {};
		}
	
		err = ExtAudioFileSeek(xaf, offset);
		if (err) {
			post("seek failed %d\n", (int)err);
			ExtAudioFileDispose(xaf);
			return {};
		}

		return new AudioToolboxSFReaderBackend(backend(xaf, numChannels));
	}	
};

typedef AudioToolboxSFReaderBackend SFReaderBackend;
#else
struct PortableBuffer {
	uint32_t numChannels;
	uint32_t size;
	void *data;

	PortableBuffer()
		: numChannels(0), size(0), data(nullptr)
	{}
};

class PortableBuffers {
public:
	PortableBuffers(int inNumChannels)
		: buffers(inNumChannels)
	{}

	~PortableBuffers() {}

	uint32_t numChannels() {
		return this->buffers.size();
	}

	void setData(size_t i, Z *data, uint32_t blockSize) {
		PortableBuffer& buf = this->buffers[i];
		buf.numChannels = 1;
		buf.size = blockSize * sizeof(Z);
		buf.data = data;
		interleaved.resize(blockSize * this->numChannels());
	}

	std::vector<PortableBuffer> buffers;
	std::vector<double> interleaved;
};

typedef PortableBuffers AudioBuffers;

class SndfileSFReaderBackend {
public:
	SndfileSFReaderBackend(SNDFILE *inSndfile, int inNumChannels)
		: mSndfile(inSndfile), mNumChannels(inNumChannels)
	{}
	
	~SndfileSFReaderBackend() {
		sf_close(this->mSndfile);
	}

	uint32_t numChannels() {
		return this->mNumChannels;
	}

	int pull(uint32_t *framesRead, AudioBuffers& buffers) {
		buffers.interleaved.resize(*framesRead * this->mNumChannels);
		sf_count_t framesReallyRead = sf_readf_double(this->mSndfile, buffers.interleaved.data(), *framesRead);

		int result = 0;
		if(framesReallyRead >= 0) {
			*framesRead = framesReallyRead;
		} else {
			*framesRead = 0;
			result = framesReallyRead;
		}

		for(int ch = 0; ch < this->mNumChannels; ch++) {
			double *buf = (double *) buffers.buffers[ch].data;
			for(sf_count_t frame = 0; frame < framesReallyRead; frame++) {
				buf[frame] = buffers.interleaved.data()[frame * this->mNumChannels + ch];
			}
		}
		
		return result;
	}
	
	SNDFILE *mSndfile;
	std::vector<double> mBufInterleaved;
	int mNumChannels;

	static SndfileSFReaderBackend *openForReading(const char *path) {
		SNDFILE *sndfile = nullptr;
		SF_INFO sfinfo = {0};
		
		if((sndfile = sf_open(path, SFM_READ, &sfinfo)) == nullptr) {
			post("failed to open file %s\n", sf_strerror(NULL));
			sf_close(sndfile);
			return nullptr;
		}

		uint32_t numChannels = sfinfo.channels;

		sf_count_t seek_result;
		if((seek_result = sf_seek(sndfile, 0, SEEK_SET) < 0)) {
			post("failed to seek file %d\n", seek_result);
			sf_close(sndfile);
			return nullptr;
		}
		
		return new SndfileSFReaderBackend(sndfile, numChannels);
	}
};

typedef SndfileSFReaderBackend SFReaderBackend;
#endif // SAPF_AUDIOTOOLBOX

class SFReaderOutputChannel;

class SFReader : public Object
{
	SFReaderBackend *mBackend;
	AudioBuffers mBuffers;
	SFReaderOutputChannel* mOutputs;
	int64_t mFramesRemaining;
	bool mFinished = false;
	
public:
	
	SFReader(SFReaderBackend *inBackend, int64_t inDuration);
	
	~SFReader();

	virtual const char* TypeName() const override { return "SFReader"; }

	P<List> createOutputs(Thread& th);
	
	bool pull(Thread& th);
	void fulfillOutputs(int blockSize);
	void produceOutputs(int shrinkBy);
};

class SFReaderOutputChannel : public Gen
{
	friend class SFReader;
	P<SFReader> mSFReader;
	SFReaderOutputChannel* mNextOutput = nullptr;
	Z* mDummy = nullptr;
	
public:	
	SFReaderOutputChannel(Thread& th, SFReader* inSFReader)
        : Gen(th, itemTypeZ, true), mSFReader(inSFReader)
	{
	}
	
	~SFReaderOutputChannel()
	{
		if (mDummy) free(mDummy);
	}
	
	virtual void norefs() override
	{
		mOut = nullptr; 
		mSFReader = nullptr;
	}
		
	virtual const char* TypeName() const override { return "SFReaderOutputChannel"; }
	
	virtual void pull(Thread& th) override
	{
		if (mSFReader->pull(th)) {
			end();
		}
	}
	
};

SFReader::SFReader(SFReaderBackend *inBackend, int64_t inDuration)
	: mBackend(inBackend), mBuffers(inBackend->numChannels()), mFramesRemaining(inDuration)
{
	
}

SFReader::~SFReader()
{
	SFReaderOutputChannel* output = mOutputs;
	do {
		SFReaderOutputChannel* next = output->mNextOutput;
		delete output;
		output = next;
	} while (output);
	delete mBackend;
}

void SFReader::fulfillOutputs(int blockSize)
{
	SFReaderOutputChannel* output = mOutputs;
	size_t bufSize = blockSize * sizeof(Z);
	for (int i = 0; output; ++i, output = output->mNextOutput){
		Z* out;
		if (output->mOut)
			out = output->mOut->fulfillz(blockSize);
		else {
			if (!output->mDummy)
				output->mDummy = (Z*)calloc(output->mBlockSize, sizeof(Z));

			out = output->mDummy;
		}

		this->mBuffers.setData(i, out, blockSize);
		memset(out, 0, bufSize);
	};
}

void SFReader::produceOutputs(int shrinkBy)
{
	SFReaderOutputChannel* output = mOutputs;
	do {
		if (output->mOut)
			output->produce(shrinkBy);
		output = output->mNextOutput;
	} while (output);
}

P<List> SFReader::createOutputs(Thread& th)
{
	const uint32_t numChannels = mBackend->numChannels();
	P<List> s = new List(itemTypeV, numChannels);
	
	// fill s->mArray with ola's output channels.
	SFReaderOutputChannel* last = nullptr;
	P<Array> a = s->mArray;
	for (uint32_t i = 0; i < numChannels; ++i) {
		SFReaderOutputChannel* c = new SFReaderOutputChannel(th, this);
		if (last) last->mNextOutput = c;
		else mOutputs = c;
		last = c;
		a->add(new List(c));
	}
	
	return s;
}

bool SFReader::pull(Thread& th)
{
	if (mFramesRemaining == 0) 
		mFinished = true;

	if (mFinished) 
		return true;
	
	SFReaderOutputChannel* output = mOutputs;
	int blockSize = output->mBlockSize;
	if (mFramesRemaining > 0)
		blockSize = (int)std::min(mFramesRemaining, (int64_t)blockSize);
	
	fulfillOutputs(blockSize);
	
	// read file here.
	uint32_t framesRead = blockSize;
	int err = mBackend->pull(&framesRead, mBuffers);
		
	if (err || framesRead == 0) {
		mFinished = true;
	}
	
	produceOutputs(blockSize - framesRead);
	if (mFramesRemaining > 0) mFramesRemaining -= blockSize;
	
	return mFinished; 
}

void sfread(Thread& th, Arg filename, int64_t offset, int64_t frames)
{
	const char* path = ((String*)filename.o())->s;

	SFReaderBackend *backend = SFReaderBackend::openForReading(path);

	if(backend != nullptr) {
		SFReader* sfr = new SFReader(backend, -1);
		th.push(sfr->createOutputs(th));
	}
}

SoundFile sfcreate(Thread& th, const char* path, int numChannels, double fileSampleRate, bool interleaved)
{
#ifdef SAPF_AUDIOTOOLBOX
	if (fileSampleRate == 0.)
		fileSampleRate = th.rate.sampleRate;

	CFStringRef cfpath = CFStringCreateWithFileSystemRepresentation(0, path);
	if (!cfpath) {
		post("failed to create path '%s'\n", path);
		return nullptr;
	}
	CFReleaser cfpathReleaser(cfpath);
	
	CFURLRef url = CFURLCreateWithFileSystemPath(0, cfpath, kCFURLPOSIXPathStyle, false);
	if (!url) {
		post("failed to create url\n");
		return nullptr;
	}
	CFReleaser urlReleaser(url);
	
	AudioStreamBasicDescription fileFormat = {
		fileSampleRate,
		kAudioFormatLinearPCM,
		kAudioFormatFlagsNativeFloatPacked,
		static_cast<UInt32>(sizeof(float) * numChannels),
		1,
		static_cast<UInt32>(sizeof(float) * numChannels),
		static_cast<UInt32>(numChannels),
		32,
		0
	};
	
	int interleavedChannels = interleaved ? numChannels : 1;
	UInt32 interleavedBit = interleaved ? 0 : kAudioFormatFlagIsNonInterleaved;
	
	AudioStreamBasicDescription clientFormat = {
		th.rate.sampleRate,
		kAudioFormatLinearPCM,
		kAudioFormatFlagsNativeFloatPacked | interleavedBit,
		static_cast<UInt32>(sizeof(float) * interleavedChannels),
		1,
		static_cast<UInt32>(sizeof(float) * interleavedChannels),
		static_cast<UInt32>(numChannels),
		32,
		0
	};
		
	ExtAudioFileRef xaf;
	OSStatus err = ExtAudioFileCreateWithURL(url, kAudioFileWAVEType, &fileFormat, nullptr, kAudioFileFlags_EraseFile, &xaf);
	
	if (err) {
		post("failed to create file '%s'. err: %d\n", path, (int)err);
		return nullptr;
	}
	
	err = ExtAudioFileSetProperty(xaf, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
	if (err) {
		post("failed to set client data format\n");
		ExtAudioFileDispose(xaf);
		return nullptr;
	}
	
	return xaf;
#else
	// TODO
        return (SoundFile) malloc(sizeof(SoundFile));
#endif // SAPF_AUDIOTOOLBOX
}

std::atomic<int32_t> gFileCount = 0;

void makeRecordingPath(Arg filename, char* path, int len)
{
	if (filename.isString()) {
		const char* recDir = getenv("SAPF_RECORDINGS");
		if (!recDir || strlen(recDir)==0) recDir = "/tmp";
		snprintf(path, len, "%s/%s.wav", recDir, ((String*)filename.o())->s);
	} else {
		int32_t count = ++gFileCount;
		snprintf(path, len, "/tmp/sapf-%s-%04d.wav", gSessionTime, count);
	}
}

void sfwrite(Thread& th, V& v, Arg filename, bool openIt)
{
#ifdef SAPF_AUDIOTOOLBOX
	std::vector<ZIn> in;
	
	int numChannels = 0;
		
	if (v.isZList()) {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of frames", "");
		numChannels = 1;
		in.push_back(ZIn(v));
	} else {
		if (!v.isFinite()) indefiniteOp(">sf : s - indefinite number of channels", "");
		P<List> s = (List*)v.o();
		s = s->pack(th);
		Array* a = s->mArray();
		numChannels = (int)a->size();

		if (numChannels > kMaxSFChannels)
			throw errOutOfRange;
		
		bool allIndefinite = true;
		for (int i = 0; i < numChannels; ++i) {
			V va = a->at(i);
			if (va.isFinite()) allIndefinite = false;
			in.push_back(ZIn(va));
			va.o = nullptr;
		}

		s = nullptr;
		a = nullptr;
		
		if (allIndefinite) indefiniteOp(">sf : s - all channels have indefinite number of frames", "");
	}
	v.o = nullptr;

	char path[1024];
	
	makeRecordingPath(filename, path, 1024);
	
	ExtAudioFileRef xaf = sfcreate(th, path, numChannels, 0., true);
	if (!xaf) return;
	
	std::valarray<float> buf(0., numChannels * kBufSize);
	AudioBufferList abl;
	abl.mNumberBuffers = 1;
	abl.mBuffers[0].mNumberChannels = numChannels;
	abl.mBuffers[0].mData = &buf[0];
	abl.mBuffers[0].mDataByteSize = kBufSize * sizeof(float);
	
	int64_t framesPulled = 0;
	int64_t framesWritten = 0;
	bool done = false;
	while (!done) {
		int minn = kBufSize;
		memset(&buf[0], 0, kBufSize * numChannels);
		for (int i = 0; i < numChannels; ++i) {
			int n = kBufSize;
			bool imdone = in[i].fill(th, n, &buf[0]+i, numChannels);
			framesPulled += n;
			if (imdone) done = true;
			minn = std::min(n, minn);
		}

		abl.mBuffers[0].mDataByteSize = minn * sizeof(float);
		OSStatus err = ExtAudioFileWrite(xaf, minn, &abl);
		if (err) {
			post("ExtAudioFileWrite failed %d\n", (int)err);
			break;
		}

		framesWritten += minn;
	}
	
	post("wrote file '%s'  %d channels  %g secs\n", path, numChannels, framesWritten * th.rate.invSampleRate);
	
	ExtAudioFileDispose(xaf);
	
	if (openIt) {
		char cmd[1100];
		snprintf(cmd, 1100, "open \"%s\"", path);
		system(cmd);
	}
#else
	// TODO
#endif // SAPF_AUDIOTOOLBOX
}
