//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: wavetrack.cpp,v 1.15.2.12 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "track.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "gconfig.h"
#include "al/dsp.h"

// Added by Tim. p3.3.18
//#define WAVETRACK_DEBUG

//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

static bool smallerZValue(Part* first, Part* second)
{
	return first->getZIndex() < second->getZIndex();
}


void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek)
{
	// Added by Tim. p3.3.17
#ifdef WAVETRACK_DEBUG
	printf("WaveTrack::fetchData %s samples:%u pos:%u\n", name().toLatin1().constData(), samples, pos);
#endif

	// reset buffer to zero
	for (int i = 0; i < channels(); ++i)
		memset(bp[i], 0, samples * sizeof (float));

	// p3.3.29
	// Process only if track is not off.
	if (!off())
	{

		PartList* pl = parts();
		//printf("Partlist Size:%d\n", (int)pl->size());
		unsigned n = samples;
		QList<Part*> sortedByZValue;
		for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			sortedByZValue.append(ip->second);
			//printf("Part name: %s\n", ip->second->name().toUtf8().constData());
		}

		qSort(sortedByZValue.begin(), sortedByZValue.end(), smallerZValue);
		//printf("Sorted List Size:%d\n", sortedByZValue.size());

		foreach(Part* wp, sortedByZValue)
		{
			WavePart* part = (WavePart*) wp;
			//WavePart* part = (WavePart*) (ip->second);
			//printf("Part name: %s\n", part->name().toUtf8().constData());
			//Find all parts under current pos, if there is a part under pos that is not
			//the current part and has a heigher zIndex than the current part skip this cycle
			//PartList* partAtPos = pl->findParts(pos, samples);

			if (part->mute())
				continue;

			unsigned p_spos = part->frame();
			unsigned p_epos = p_spos + part->lenFrame();
			if (pos + n < p_spos)
			{
				continue;
				//break;
			}
			if (pos >= p_epos)
				continue;

			EventList* events = part->events();
			for (iEvent ie = events->begin(); ie != events->end(); ++ie)
			{
				Event& event = ie->second;
				unsigned e_spos = event.frame() + p_spos;
				unsigned nn = event.lenFrame();
				unsigned e_epos = e_spos + nn;

				if (pos + n < e_spos)
					break;
				if (pos >= e_epos)
					continue;

				int offset = e_spos - pos;

				unsigned srcOffset, dstOffset;
				if (offset > 0)
				{
					nn = n - offset;
					srcOffset = 0;
					dstOffset = offset;
				}
				else
				{
					srcOffset = -offset;
					dstOffset = 0;

					nn += offset;
					if (nn > n)
						nn = n;
				}
				float* bpp[channels()];
				for (int i = 0; i < channels(); ++i)
					bpp[i] = bp[i] + dstOffset;

				// By T356. Allow overlapping parts or events to mix together !
				// Since the buffers are cleared above, just read and add (don't overwrite) the samples.
				//event.read(srcOffset, bpp, channels(), nn);
				//event.read(srcOffset, bpp, channels(), nn, false);
				//event.readAudio(srcOffset, bpp, channels(), nn, doSeek, false);
				// p3.3.33
				//FIXME: this is where the audio is overwriting
				//printf("Source offset: %u sample count: %u\n", srcOffset, nn);
				event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, true);

			}
		}
		//printf("\n");
	}

	if (config.useDenormalBias)
	{
		// add denormal bias to outdata
		for (int i = 0; i < channels(); ++i)
			for (unsigned int j = 0; j < samples; ++j)
			{
				bp[i][j] += denormalBias;
			}
	}

	// p3.3.41
	//fprintf(stderr, "WaveTrack::fetchData data: samples:%ld %e %e %e %e\n", samples, bp[0][0], bp[0][1], bp[0][2], bp[0][3]);

	_prefetchFifo.add();
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveTrack::write(int level, Xml& xml) const
{
	xml.tag(level++, "wavetrack");
	AudioTrack::writeProperties(level, xml);
	const PartList* pl = cparts();
	for (ciPart p = pl->begin(); p != pl->end(); ++p)
		p->second->write(level, xml);
	xml.etag(level, "wavetrack");
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveTrack::read(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "part")
				{
					//Part* p = newPart();
					//p->read(xml);
					Part* p = 0;
					p = readXmlPart(xml, this);
					if (p)
						parts()->add(p);
				}
				else if (AudioTrack::readProperties(xml, tag))
					xml.unknown("WaveTrack");
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if (tag == "wavetrack")
				{
					mapRackPluginsToControllers();
					return;
				}
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* WaveTrack::newPart(Part*p, bool clone)
{
	WavePart* part = clone ? new WavePart(this, p->events()) : new WavePart(this);
	if (p)
	{
		part->setName(p->name());
		part->setColorIndex(getDefaultPartColor());

		*(PosLen*) part = *(PosLen*) p;
		part->setMute(p->mute());
	}

	if (clone)
	{
		//p->chainClone(part);
		chainClone(p, part);
		part->setColorIndex(p->colorIndex());
		part->setZIndex(p->getZIndex());
	}

	return part;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int channels, unsigned nframe, float** bp)
{
	//if(debugMsg)
	//  printf("WaveTrack::getData framePos:%u channels:%d nframe:%u processed?:%d\n", framePos, channels, nframe, processed());

	if ((song->bounceTrack != this) && !noInRoute())
	{
		RouteList* irl = inRoutes();
		iRoute i = irl->begin();
		if (i->track->isMidiTrack())
		{
			if (debugMsg)
				printf("WaveTrack::getData: Error: First route is a midi track route!\n");
			return false;
		}
		// p3.3.38
		//((AudioTrack*)i->track)->copyData(framePos, channels, nframe, bp);
		((AudioTrack*) i->track)->copyData(framePos, channels,
				//(i->track->type() == Track::AUDIO_SOFTSYNTH && i->channel != -1) ? i->channel : 0,
				i->channel,
				i->channels,
				nframe, bp);

		++i;
		for (; i != irl->end(); ++i)
		{
			if (i->track->isMidiTrack())
			{
				if (debugMsg)
					printf("WaveTrack::getData: Error: Route is a midi track route!\n");
				//return false;
				continue;
			}
			// p3.3.38
			//((AudioTrack*)i->track)->addData(framePos, channels, nframe, bp);
			((AudioTrack*) i->track)->addData(framePos, channels,
					//(i->track->type() == Track::AUDIO_SOFTSYNTH && i->channel != -1) ? i->channel : 0,
					i->channel,
					i->channels,
					nframe, bp);

		}
		if (recordFlag())
		{
			if (audio->isRecording() && recFile())
			{
				if (audio->freewheel())
				{
				}
				else
				{
					if (fifo.put(channels, nframe, bp, audio->pos().frame()))
						printf("WaveTrack::getData(%d, %d, %d): fifo overrun\n",
							framePos, channels, nframe);
				}
			}
			return true;
		}
	}
	if (!audio->isPlaying())
		return false;

	//printf("WaveTrack::getData no out routes\n");

	if (audio->freewheel())
	{

		// when freewheeling, read data direct from file:
		// Indicate do not seek file before each read.
		// Changed by Tim. p3.3.17
		//fetchData(framePos, nframe, bp);
		fetchData(framePos, nframe, bp, false);

	}
	else
	{
		unsigned pos;
		if (_prefetchFifo.get(channels, nframe, bp, &pos))
		{
			printf("WaveTrack::getData(%s) fifo underrun\n",
					name().toLatin1().constData());
			return false;
		}
		if (pos != framePos)
		{
			if (debugMsg)
				printf("fifo get error expected %d, got %d\n",
					framePos, pos);
			while (pos < framePos)
			{
				if (_prefetchFifo.get(channels, nframe, bp, &pos))
				{
					printf("WaveTrack::getData(%s) fifo underrun\n",
							name().toLatin1().constData());
					return false;
				}
			}
		}

		// p3.3.41
		//fprintf(stderr, "WaveTrack::getData %s data: nframe:%ld %e %e %e %e\n", name().toLatin1().constData(), nframe, bp[0][0], bp[0][1], bp[0][2], bp[0][3]);

	}
	//}
	return true;
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void WaveTrack::setChannels(int n)
{
	AudioTrack::setChannels(n);
	SndFile* sf = recFile();
	if (sf)
	{
		if (sf->samples() == 0)
		{
			sf->remove();
			sf->setFormat(sf->format(), _channels,
					sf->samplerate());
			sf->openWrite();
		}
	}
}

