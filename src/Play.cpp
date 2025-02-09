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

#include "Play.hpp"
#if defined(SAPF_AUDIOTOOLBOX)
#include <AudioToolbox/AudioToolbox.h>
#else)
#include <RtAudio.h>
#endif
#include <pthread.h>
#include <atomic>
#include <thread>

#include "SoundFiles.hpp"

#if defined(SAPF_AUDIOTOOLBOX)
static OSStatus inputCallback(
	void *inRefCon,
	AudioUnitRenderActionFlags *ioActionFlags,
	const AudioTimeStamp *inTimeStamp,
	UInt32 inBusNumber,
	UInt32 inNumberFrames,
	AudioBufferList *ioData
);

struct AUPlayerBackend
{
	AUPlayerBackend(int inNumChannels, ExtAudioFileRef inXAFRef = nullptr)
		: player(nullptr), numChannels(inNumChannels), outputUnit(nullptr), xaf(inXAFRef)
	{}
	
	~AUPlayerBackend() {
		if (xaf) {
			ExtAudioFileDispose(xaf);
			char cmd[1100];
			snprintf(cmd, 1100, "open \"%s\"", path.c_str());
			system(cmd);
		}
	}

	int32_t createGraph() {
		OSStatus err = noErr;
		AudioComponentInstance outputUnit = openAU('auou', 'def ', 'appl');
		if (!outputUnit) {
			post("open output unit failed\n");
			return 'fail';
		}
	
		this->outputUnit = outputUnit;
	
		UInt32 flags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
		AudioStreamBasicDescription fmt = { vm.ar.sampleRate, kAudioFormatLinearPCM, flags, 4, 1, 4, (Uint32)this->numChannels, 32, 0 };

		// mFormatID = 1819304813
		// mFormatFlags = 41
		// mSampleRate = 96000
		// mBitsPerChannel = 32
		// mBytesPerFrame = 4
		// mChannelsPerFrame = 1
		// mBytesPerPacket = 4
		// mFramesPerPacket = 1
	
		err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &fmt, sizeof(fmt));
		if (err) {
			post("set outputUnit client format failed\n");
			return err;
		}
	
		AURenderCallbackStruct cbs;
		
		cbs.inputProc = inputCallback;
		cbs.inputProcRefCon = this->player;
	
		err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cbs, sizeof(cbs));
		if (err) {
			post("set render callback failed\n");
			return err;
		}
	
		err = AudioUnitInitialize(outputUnit);
		if (err) {
			post("initialize output unit failed\n");
			return err;
		}
	
		err = AudioOutputUnitStart(outputUnit);
		if (err) {
			post("start output unit failed\n");
			return err;
		}
	
		post("start output unit OK\n");
	
		return noErr;
	}

	void *player;
	int numChannels;
	AudioComponentInstance outputUnit;
	ExtAudioFileRef xaf = nullptr;
};

typedef AUPlayerBackend PlayerBackend;

class AUBuffers {
public:
	AUBuffers(AudioBufferList *inIoData)
		: ioData(inIoData)
	{}
	
	uint32_t count() {
		return this->ioData->mNumberBuffers;
	}
	
	float *data(int channel) {
		return (float*) this->ioData->mBuffers[channel].mData;
	}

	uint32_t size(int channel) {
		return this->ioData->mBuffers[channel].mDataByteSize;
	}
	
	AudioBufferList *ioData;
};

typedef AUBuffers Buffers;
#else
int rtPlayerBackendCallback(
	void *outputBuffer,
	void *inputBuffer,
	unsigned int nBufferFrames,
	double streamTime,
	RtAudioStreamStatus status,
	void *userData
);

class RtPlayerBackend {
public:
	RtPlayerBackend(int inNumChannels)
		: player(nullptr), numChannels(inNumChannels)
	{}
	
	int32_t createGraph() {
		RtAudio dac;
		if(dac.getDeviceCount() < 1) {
			std::cout << "\nNo audio devices found!\n";
			exit(0);
		}

		RtAudio::StreamParameters parameters;
		parameters.deviceId = dac.getDefaultOutputDevice();
		parameters.nChannels = this->numChannels;
		parameters.firstChannel = 0;
		unsigned int sampleRate = vm.ar.sampleRate;
		unsigned int bufferFrames = 256; // 256 sample frames
		RtAudio::StreamOptions options;
		options.flags = RTAUDIO_NONINTERLEAVED /* | RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME */;
 
		dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &rtPlayerBackendCallback, this->player, &options);
		dac.startStream();

		post("start output unit OK\n");
   
		char input;
		std::cout << "\n<press return to stop>\n";
		std::cin.get(input);
 
		// // Block released ... stop the stream
		// if(dac.isStreamRunning()) {
		// 	dac.stopStream();  // or could call dac.abortStream();
		// }
 
	// cleanup:
	// 	if(dac.isStreamOpen()) {
	// 		dac.closeStream();
	// 	}
 
		return 0;
	}

	void *player;
	int numChannels;
	// RtAudio audio;
};

typedef RtPlayerBackend PlayerBackend;

class RtBuffers {
public:
	RtBuffers(float *inOut, uint32_t inCount, uint32_t inSize)
		: out(inOut), theCount(inCount), theSize(inSize)
	{}
	
	uint32_t count() {
		return this->theCount;
	}
	
	float *data(int channel) {
		return this->out + channel * this->theSize;
	}

	uint32_t size(int channel) {
		return this->theSize;
	}
	
	float *out;
	uint32_t theCount;
	uint32_t theSize;
};

typedef RtBuffers Buffers;
#endif

const int kMaxChannels = 32;

struct Player {
	Player(Thread& inThread, PlayerBackend inBackend);
	~Player();

	int numChannels();
	int32_t createGraph();
	
	Thread th;
	int count; // unused?
	bool done;
	Player* prev;
	Player* next;
	PlayerBackend backend;
	// AudioComponentInstance outputUnit;
	ZIn in[kMaxChannels];
	// ExtAudioFileRef xaf = nullptr;
	std::string path; // recording file path
};

static bool fillBufferList(Player *player, int inNumberFrames, Buffers *buffers);

struct Player* gAllPlayers = nullptr;

Player::Player(Thread& inThread, PlayerBackend inBackend)
	: th(inThread), count(0), done(false), prev(nullptr), next(gAllPlayers), backend(inBackend)
{ 
	gAllPlayers = this; 
	if (next) next->prev = this; 
}

Player::~Player() {
	if (next) next->prev = prev;
	
	if (prev) prev->next = next;
	else gAllPlayers = next;
		
	// if (xaf) {
	//     ExtAudioFileDispose(xaf);
	//     char cmd[1100];
	//     snprintf(cmd, 1100, "open \"%s\"", path.c_str());
	//     system(cmd);
	// }
}

int Player::numChannels() {
	return this->backend.numChannels;
}

int32_t Player::createGraph() {
	return this->backend.createGraph();
}

pthread_mutex_t gPlayerMutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef SAPF_AUDIOTOOLBOX
static OSStatus inputCallback(	void *							inRefCon,
	AudioUnitRenderActionFlags *	ioActionFlags,
	const AudioTimeStamp *			inTimeStamp,
	UInt32							inBusNumber,
	UInt32							inNumberFrames,
	AudioBufferList *				ioData)
{
	
	Player* player = (Player*)inRefCon;
	AUBuffers buffers(ioData);
		
	bool done = fillBufferList(player, inNumberFrames, buffers);
	recordPlayer(player, inNumberFrames, ioData);

	if (done) {
		player->done = true;
	}
	return noErr;
}

static void stopPlayer(AUPlayer* player)
{
	AudioComponentInstance outputUnit = player->outputUnit;
	player->outputUnit = nullptr;
	if (outputUnit) {
	
		OSStatus err = AudioOutputUnitStop(outputUnit);
		if (err) post("AudioOutputUnitStop err %d\n", (int)err);
		err = AudioComponentInstanceDispose(outputUnit);
		if (err) post("AudioComponentInstanceDispose outputUnit err %d\n", (int)err);
		
	}
	delete player;
}
#else
int rtPlayerBackendCallback(
	void *outputBuffer,
	void *inputBuffer,
	unsigned int nBufferFrames,
	double streamTime,
	RtAudioStreamStatus status,
	void *userData
) {
	float *out = (float *) outputBuffer;
	Player *player = (Player *) userData;
	RtBuffers buffers((float *) outputBuffer, player->numChannels(), nBufferFrames);
 
	if(status) {
		std::cout << "Stream underflow detected!" << std::endl;
	}

	bool done = fillBufferList(player, nBufferFrames, &buffers);
	// recordPlayer(player, inNumberFrames, ioData);

	if (done) {
		player->done = true;
	}
	return 0;
}
#endif

void stopPlaying()
{
#ifdef SAPF_AUDIOTOOLBOX
	Locker lock(&gPlayerMutex);

	AUPlayer* player = gAllPlayers;
	while (player) {
		AUPlayer* next = player->next;
		stopPlayer(player);
		player = next;
	}
#else
        // TODO: cross platform playback
#endif // SAPF_AUDIOTOOLBOX
}

void stopPlayingIfDone()
{
#ifdef SAPF_AUDIOTOOLBOX
    Locker lock(&gPlayerMutex);
	
	AUPlayer* player = gAllPlayers;
	while (player) {
		AUPlayer* next = player->next;
		if (player->done)
			stopPlayer(player);
		player = next;
	}
#else
        // TODO: cross platform playback
#endif // SAPF_AUDIOTOOLBOX
}

static bool fillBufferList(Player *player, int inNumberFrames, Buffers *buffers)
{
	if (player->done) {
	zeroAll:
		for (int i = 0; i < (int)buffers->count(); ++i) {
			memset(buffers->data(i), 0, inNumberFrames * sizeof(float));
		}
		return true;
	}
	ZIn* in = player->in;
	bool done = true;
	for (int i = 0; i < (int)buffers->count(); ++i) {
		int n = inNumberFrames;
		if (i >= player->numChannels()) {
			memset(buffers->data(i), 0, buffers->size(i));
		} else {
			try {
				float* buf = buffers->data(i);
				bool imdone = in[i].fill(player->th, n, buf, 1);
				if (n < inNumberFrames) {
					memset(buffers->data(i) + n, 0, (inNumberFrames - n) * sizeof(float));
				}
				done = done && imdone;
			} catch (int err) {
				if (err <= -1000 && err > -1000 - kNumErrors) {
					post("\nerror: %s\n", errString[-1000 - err]);
				} else {
					post("\nerror: %d\n", err);
				}
				post("exception in real time. stopping player.\n");
				done = true;
				goto zeroAll;
			} catch (std::bad_alloc& xerr) {
				post("\nnot enough memory\n");
				post("exception in real time. stopping player.\n");
				done = true;
				goto zeroAll;
			} catch (...) {
				post("\nunknown error\n");
				post("exception in real time. stopping player.\n");
				done = true;
				goto zeroAll;
			}
		}
	}
	
	return done;
}

bool gWatchdogRunning = false;
pthread_t watchdog;

static void* stopDonePlayers(void* x)
{
	using namespace std::chrono_literals;
	
	while(1) {
		std::this_thread::sleep_for(1s);
		stopPlayingIfDone();
	}
	return nullptr;
}

#ifdef SAPF_AUDIOTOOLBOX

static void recordPlayer(AUPlayer* player, int inNumberFrames, AudioBufferList const* inData)
{
	if (!player->xaf) return;
		
	OSStatus err = ExtAudioFileWriteAsync(player->xaf, inNumberFrames, inData); // initialize async.
	if (err) printf("ExtAudioFileWriteAsync err %d\n", (int)err);
}


static AudioComponentInstance openAU(UInt32 inType, UInt32 inSubtype, UInt32 inManuf)
{
    AudioComponentDescription desc;
    desc.componentType = inType;
    desc.componentSubType = inSubtype;
    desc.componentManufacturer = inManuf;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
	if (!comp) {
		return nullptr;
	}

    AudioComponentInstance au = nullptr;
    AudioComponentInstanceNew(comp, &au);
	
	return au;
}

#endif // SAPF_AUDIOTOOLBOX

void playWithAudioUnit(Thread& th, V& v)
{
	if (!v.isList()) wrongType("play : s", "List", v);

	Locker lock(&gPlayerMutex);
	
	Player *player;
	
	if (v.isZList()) {
		PlayerBackend backend(1);
		player = new Player(th, backend);
		player->backend.player = player;
		player->in[0].set(v);
	} else {
		if (!v.isFinite()) indefiniteOp("play : s", "");
		P<List> s = (List*)v.o();
		s = s->pack(th, kMaxChannels);
		if (!s()) {
			post("Too many channels. Max is %d.\n", kMaxChannels);
			return;
		}
		Array* a = s->mArray();
		
		int asize = (int)a->size();
		
		PlayerBackend backend(asize);
		player = new Player(th, backend);
		player->backend.player = player;
		for (int i = 0; i < asize; ++i) {
			player->in[i].set(a->at(i));
		}
		s = nullptr;
		a = nullptr;
	}
	v.o = nullptr; // try to prevent leak.
    
	std::atomic_thread_fence(std::memory_order_seq_cst);
		
	if (!gWatchdogRunning) {
		pthread_create(&watchdog, nullptr, stopDonePlayers, nullptr);
		gWatchdogRunning = true;
	}

	{
		int32_t err = 0;
		err = player->createGraph();
		if (err) {
			post("play failed: %d '%4.4s'\n", (int)err, (char*)&err);
			throw errFailed;
		}
	}
}


void recordWithAudioUnit(Thread& th, V& v, Arg filename)
{
#ifdef SAPF_AUDIOTOOLBOX
	if (!v.isList()) wrongType("play : s", "List", v);

	Locker lock(&gPlayerMutex);
	
	AUPlayer *player;

	char path[1024];
	ExtAudioFileRef xaf = nullptr;
	
	if (v.isZList()) {
		makeRecordingPath(filename, path, 1024);
		xaf = sfcreate(th, path, 1, 0., false);
		if (!xaf) {
			printf("couldn't create recording file \"%s\"\n", path);
			return;
		}

		player = new AUPlayer(th, 1, xaf);
		player->in[0].set(v);
		player->numChannels = 1;
	} else {
		if (!v.isFinite()) indefiniteOp("play : s", "");
		P<List> s = (List*)v.o();
		s = s->pack(th, kMaxChannels);
		if (!s()) {
			post("Too many channels. Max is %d.\n", kMaxChannels);
			return;
		}
		Array* a = s->mArray();
		
		int numChannels = (int)a->size();

		makeRecordingPath(filename, path, 1024);
		xaf = sfcreate(th, path, numChannels, 0., false);
		if (!xaf) {
			printf("couldn't create recording file \"%s\"\n", path);
			return;
		}
		
		player = new AUPlayer(th, numChannels, xaf);
		for (int i = 0; i < numChannels; ++i) {
			player->in[i].set(a->at(i));
		}
		s = nullptr;
		a = nullptr;
	}
	v.o = nullptr; // try to prevent leak.

	player->path = path;

	{
		OSStatus err = ExtAudioFileWriteAsync(xaf, 0, nullptr); // initialize async.
		if (err) printf("init ExtAudioFileWriteAsync err %d\n", (int)err);
    }
	
    std::atomic_thread_fence(std::memory_order_seq_cst);
		
	if (!gWatchdogRunning) {
		pthread_create(&watchdog, nullptr, stopDonePlayers, nullptr);
		gWatchdogRunning = true;
	}

	{
		OSStatus err = noErr;
		err = player->createGraph();
		if (err) {
			post("play failed: %d '%4.4s'\n", (int)err, (char*)&err);
			throw errFailed;
		}
	}
#else
        // TODO: cross platform playback
#endif // SAPF_AUDIOTOOLBOX
}

