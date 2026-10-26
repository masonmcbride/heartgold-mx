/*
 * SSEQ Player - Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-18
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#pragma once

#include <memory>
#include <bitset>
#include "SSEQ.h"
#include "Track.h"
#include "Channel.h"
#include "consts.h"

struct Player
{
	uint8_t prio, nTracks;
	uint16_t tempo, tempoCount, tempoRate /* 8.8 fixed point */;
	int16_t masterVol, sseqVol;

	const SSEQ *sseq;

	uint8_t trackIds[FSS_TRACKCOUNT];
	Track tracks[FSS_MAXTRACKS];
	Channel channels[16];
	int16_t variables[32];

	uint32_t sampleRate;
	Interpolation interpolation;

	Player();

	bool Setup(const SSEQ *sseq);
	void ClearState();
	void FreeTracks();
	void Stop(bool bKillSound);
	int ChannelAlloc(int type, int prio, uint16_t mask=0xffff, bool strong=false);
	// Optional shared DS physical-channel owner supplied by ds_audio.
	std::function<int(Player *,int,int,uint16_t,bool)> channelAllocator;
	uint16_t startChannelMask=0xffff;
	bool startChannelMaskEnabled=false;
	int TrackAlloc();
	void Run();
	void UpdateTracks();
	void Timer();

	/* Playback helper */
	double secondsPerSample, secondsIntoPlayback, secondsUntilNextClock;
	std::bitset<16> mutes;
	void GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples);
};
