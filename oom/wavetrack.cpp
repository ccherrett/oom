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
#include "FadeCurve.h"
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

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek)
{
	// Added by Tim. p3.3.17
#ifdef WAVETRACK_DEBUG
	printf("WaveTrack::fetchData %s samples:%u pos:%u\n", name().toLatin1().constData(), samples, pos);
#endif

	// reset buffer to zero
	for (int i = 0; i < channels(); ++i)
	{
		memset(bp[i], 0, samples * sizeof (float));
	}


	// p3.3.29
	// Process only if track is not off.
	if (!off())
	{

		PartList* pl = parts();
		unsigned n = samples;
		QList<Part*> sortedByZValue;
		for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			sortedByZValue.append(ip->second);
		}

		qSort(sortedByZValue.begin(), sortedByZValue.end(), Part::smallerZValue);
		//printf("Sorted List Size:%d\n", sortedByZValue.size());

		foreach(Part* wp, sortedByZValue)
		{
			WavePart* part = (WavePart*) wp;

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
			{
				continue;
			}

			//we now only support a single event per wave part so no need for iteration
			//EventList* events = part->events();
			iEvent ie = part->events()->begin();
			if(ie != part->events()->end())
			{
				Event& event = ie->second;
				unsigned e_spos = event.frame() + p_spos;
				unsigned nn = event.lenFrame();
				unsigned e_epos = e_spos + nn;

				if (pos + n >= e_spos && pos < e_epos)
				{
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

					//Read in samples, since the parts are now processed via zIndex and 
					//not left to right we can overwrite as we always want to hear the part on top
					//Set false here to mix all layers togather, maybe we should make this a user
					//setting.
					//printf("WaveTrack::fetchData %s samples:%u pos:%u pstart:%u srcOffset:%u\n", name().toLatin1().constData(), samples, pos, p_spos, srcOffset);
					event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, true);
				}
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
					calculateCrossFades();
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
			printf("WaveTrack::getData(%s) fifo underrun\n", name().toLatin1().constData());
			return false;
		}
		if (pos != framePos)
		{
			if (debugMsg)
				printf("fifo get error expected %d, got %d\n", framePos, pos);
			while (pos < framePos)
			{
				if (_prefetchFifo.get(channels, nframe, bp, &pos))
				{
					printf("WaveTrack::getData(%s) fifo underrun\n", name().toLatin1().constData());
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
			sf->setFormat(sf->format(), _channels, sf->samplerate());
			sf->openWrite();
		}
	}
}

static const int CROSSFADE_WIDTH = 256;

void WaveTrack::calculateCrossFades()
{
	QList<WavePart*> sortedByZValue;
	for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
	{
		WavePart* wp = (WavePart*)ip->second;
		wp->crossFadeIn()->setWidth(0);
		wp->crossFadeOut()->setWidth(0);
		sortedByZValue.append(wp);
	}

	qSort(sortedByZValue.begin(), sortedByZValue.end(), Part::smallerZValue);

	foreach(WavePart* wp, sortedByZValue)
	{
		foreach(WavePart* partWithHigherZindex, sortedByZValue)
		{
			if (partWithHigherZindex->getZIndex() > wp->getZIndex() &&
			    partWithHigherZindex->frame() > wp->frame() &&
			    partWithHigherZindex->frame() < wp->endFrame())
			{
				unsigned bottomPartFadeOutStart = partWithHigherZindex->frame() - wp->frame();

				wp->crossFadeOut()->setWidth(CROSSFADE_WIDTH);
				wp->crossFadeOut()->setFrame(bottomPartFadeOutStart);

				partWithHigherZindex->crossFadeIn()->setWidth(CROSSFADE_WIDTH);
			}

			if (partWithHigherZindex->getZIndex() > wp->getZIndex() &&
			    partWithHigherZindex->endFrame() > wp->frame() &&
			    partWithHigherZindex->endFrame() < wp->endFrame())
			{
				unsigned bottomPartFadeInStart = partWithHigherZindex->endFrame() - wp->frame() - CROSSFADE_WIDTH;

				wp->crossFadeIn()->setWidth(CROSSFADE_WIDTH);
				wp->crossFadeIn()->setFrame(bottomPartFadeInStart);

				partWithHigherZindex->crossFadeOut()->setWidth(CROSSFADE_WIDTH);
			}
		}
	}

}
