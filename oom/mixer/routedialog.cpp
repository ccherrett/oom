//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: routedialog.cpp,v 1.5.2.2 2007/01/04 00:35:17 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QCloseEvent>
#include <QDialog>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QList>

#include "routedialog.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "driver/jackaudio.h"

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
: QDialog(parent)
{
	setupUi(this);
	connect(routeList, SIGNAL(itemSelectionChanged()), SLOT(routeSelectionChanged()));
	connect(newSrcList, SIGNAL(itemSelectionChanged()), SLOT(srcSelectionChanged()));
	connect(newDstList, SIGNAL(itemSelectionChanged()), SLOT(dstSelectionChanged()));
	connect(tracksList, SIGNAL(itemSelectionChanged()), SLOT(trackSelectionChanged()));
	connect(removeButton, SIGNAL(clicked()), SLOT(removeRoute()));
	connect(connectButton, SIGNAL(clicked()), SLOT(addRoute()));
	connect(btnConnectOut, SIGNAL(clicked()), SLOT(addOutRoute()));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	routingChanged();
}

//---------------------------------------------------------
//   routingChanged
//---------------------------------------------------------

void RouteDialog::routingChanged()
{
	//---------------------------------------------------
	//  populate lists
	//---------------------------------------------------

	routeList->clear();
	newSrcList->clear();
	newDstList->clear();
	tracksList->clear();
	btnConnectOut->setEnabled(false);
	connectButton->setEnabled(false);
	removeButton->setEnabled(false);

	TrackList* tl = song->tracks();
	for (ciTrack i = tl->begin(); i != tl->end(); ++i)
	{
		if ((*i)->isMidiTrack())
			continue;
		// p3.3.38
		//WaveTrack* track = (WaveTrack*)(*i);
		AudioTrack* track = (AudioTrack*) (*i);
		//tracksList->addItem(track->name());
		/*if (track->type() == Track::AUDIO_INPUT)
		{
			for (int channel = 0; channel < track->channels(); ++channel)
				newDstList->addItem(Route(track, channel).name());
			const RouteList* rl = track->inRoutes();
			for (ciRoute r = rl->begin(); r != rl->end(); ++r)
			{
				//Route dst(track->name(), true, r->channel);
				Route dst(track->name(), true, r->channel, Route::TRACK_ROUTE);
				new QTreeWidgetItem(routeList, QStringList() << r->name() << dst.name());
			}
		}*/
		//else if (track->type() != Track::AUDIO_AUX)
		//	newDstList->addItem(Route(track, -1).name());
		if (track->type() == Track::AUDIO_OUTPUT)
		{
			for (int channel = 0; channel < track->channels(); ++channel)
			{
				Route r(track, channel);
				tracksList->addItem(r.name());
				//newSrcList->addItem(r.name());
			}
		}
		else
			tracksList->addItem(Route(track, -1).name());
			//newSrcList->addItem(Route(track, -1).name());
/*
		const RouteList* rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			QString src(track->name());
			if (track->type() == Track::AUDIO_OUTPUT)
			{
				Route s(src, false, r->channel);
				src = s.name();
			}
			new QTreeWidgetItem(routeList, QStringList() << src << r->name());
		}
*/
	}
/*	if (!checkAudioDevice()) return;
	std::list<QString> sl = audioDevice->outputPorts();
	for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
		newSrcList->addItem(*i);
	sl = audioDevice->inputPorts();
	for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
		newDstList->addItem(*i);
	routeSelectionChanged(); // init remove button
	srcSelectionChanged(); // init select button*/
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void RouteDialog::songChanged(int v)
{
	if (v & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_ROUTE))
	{
		routingChanged();
	}
}

//---------------------------------------------------------
//   routeSelectionChanged
//---------------------------------------------------------

void RouteDialog::routeSelectionChanged()
{
	QTreeWidgetItem* item = routeList->currentItem();
	removeButton->setEnabled(item != 0);
}

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteDialog::removeRoute()
{
	QTreeWidgetItem* item = routeList->currentItem();
	if (item == 0)
		return;
	audio->msgRemoveRoute(Route(item->text(0), false, -1), Route(item->text(1), true, -1));
	audio->msgUpdateSoloStates();
	//song->update(SC_SOLO);
	song->update(SC_ROUTE);
	delete item;
}

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void RouteDialog::addRoute()/*{{{*/
{
	QListWidgetItem* srcItem = newSrcList->currentItem();
	QListWidgetItem* tItem = tracksList->currentItem();
	if (!_selected || srcItem == 0)
		return;
	int chan = 0;
	if (_selected->type() == Track::AUDIO_INPUT)
	{
		if(!tItem)
			return;
		int row = tracksList->row(tItem);
		QList<QListWidgetItem*> found = tracksList->findItems(tItem->text(), Qt::MatchExactly);
		if(found.isEmpty())
			return;
		for(int i = 0; i < found.size(); ++i)
		{
			QListWidgetItem* item = found.at(i);
			chan = i;
			int r = tracksList->row(item);
			if(r == row)
				break;
		}

		Route srcRoute(srcItem->text(), false, -1, Route::JACK_ROUTE);
		Route dstRoute(_selected, chan);

		srcRoute.channel = chan;

		audio->msgAddRoute(srcRoute, dstRoute);

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
		//new QTreeWidgetItem(routeList, QStringList() << srcItem->text() << tItem->text());
		connectButton->setEnabled(false);
		return;
	}

		Route dstRoute(_selected, chan, _selected->channels());
		Route srcRoute(srcItem->text(), true, -1);
		//dstRoute.channel = chan;
		//Route &srcRoute = imm->second;

		//Route dstRoute(t, imm->second.channel, imm->second.channels);
		//dstRoute.remoteChannel = imm->second.remoteChannel;

		audio->msgAddRoute(srcRoute, dstRoute);
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
		//new QTreeWidgetItem(routeList, QStringList() << srcItem->text() << tItem->text());
		connectButton->setEnabled(false);
	/*
	audio->msgAddRoute(Route(srcItem->text(), false, -1), Route(dstItem->text(), true, -1));
	audio->msgUpdateSoloStates();
	//song->update(SC_SOLO);
	song->update(SC_ROUTE);
	new QTreeWidgetItem(routeList, QStringList() << srcItem->text() << dstItem->text());
	connectButton->setEnabled(false);*/
}/*}}}*/

void RouteDialog::addOutRoute()/*{{{*/
{
	QListWidgetItem* dstItem = newDstList->currentItem();
	QListWidgetItem* tItem = tracksList->currentItem();
	if (!_selected || dstItem == 0)
		return;

	RouteList* rl = _selected->outRoutes();
	int chan = 0;

	if (_selected->type() == Track::AUDIO_OUTPUT)
	{
		if(!tItem)
			return;
		int row = tracksList->row(tItem);
		QList<QListWidgetItem*> found = tracksList->findItems(tItem->text(), Qt::MatchExactly);
		if(found.isEmpty())
			return;
		for(int i = 0; i < found.size(); ++i)
		{
			QListWidgetItem* item = found.at(i);
			chan = i;
			int r = tracksList->row(item);
			if(r == row)
				break;
		}

		Route srcRoute(_selected, chan);
		Route dstRoute(dstItem->text(), true, -1, Route::JACK_ROUTE);
		dstRoute.channel = chan;

		// check if route src->dst exists:
		iRoute irl = rl->begin();
		for (; irl != rl->end(); ++irl)
		{
			if (*irl == dstRoute)
				break;
		}
		audio->msgAddRoute(srcRoute, dstRoute);
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
		//new QTreeWidgetItem(routeList, QStringList() << srcRoute.name() << dstRoute.name());
		btnConnectOut->setEnabled(false);
		return;
	}
	else
	{
		Route srcRoute(_selected, chan, _selected->channels());
		Route dstRoute(dstItem->text(), true, -1);

		audio->msgAddRoute(srcRoute, dstRoute);
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
		//new QTreeWidgetItem(routeList, QStringList() << srcRoute.name() << dstRoute.name());
		btnConnectOut->setEnabled(false);
	}
	/*
	audio->msgAddRoute(Route(srcItem->text(), false, -1), Route(dstItem->text(), true, -1));
	audio->msgUpdateSoloStates();
	//song->update(SC_SOLO);
	song->update(SC_ROUTE);
	new QTreeWidgetItem(routeList, QStringList() << srcItem->text() << dstItem->text());
	connectButton->setEnabled(false);
	*/
}/*}}}*/


void RouteDialog::setSourceSelection(QString src)
{
	newSrcList->setCurrentRow(-1);
	QList<QListWidgetItem*> found = newSrcList->findItems(src, Qt::MatchExactly);
	if(found.isEmpty())
		return;
	for(int i = 0; i < found.size(); ++i)
	{
		newSrcList->setCurrentItem(found.at(i), QItemSelectionModel::ClearAndSelect);
	}
	newDstList->setCurrentRow(-1);
}

void RouteDialog::setDestSelection(QString dest)
{
	newDstList->setCurrentRow(-1);
	QList<QListWidgetItem*> found = newDstList->findItems(dest, Qt::MatchExactly);
	if(found.isEmpty())
		return;
	for(int i = 0; i < found.size(); ++i)
	{
		newDstList->setCurrentItem(found.at(i), QItemSelectionModel::ClearAndSelect);
	}
	newSrcList->setCurrentRow(-1);
}

void RouteDialog::trackSelectionChanged()
{
	routeList->clear();
	newSrcList->clear();
	newDstList->clear();
	QListWidgetItem* titem = tracksList->currentItem();
	AudioTrack* atrack = (AudioTrack*)song->findTrack(titem->text());
	if(atrack)
	{
		_selected = atrack;
		TrackList* tl = song->tracks();
		for(iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			if((*t)->isMidiTrack())
				continue;
			AudioTrack* track = (AudioTrack*) (*t);
			if(track->name() == atrack->name())
				continue; //You cant connect a track to itself
			//int channels = track->channels();
			switch (track->type())
			{
				case Track::AUDIO_OUTPUT:/*{{{*/
					for (int channel = 0; channel < track->channels(); ++channel)
					{
						Route r(track, channel);
						newSrcList->addItem(r.name());
					}
					newDstList->addItem(Route(track, -1).name());
				break;
				case Track::AUDIO_AUX:
					newDstList->addItem(Route(track, -1).name());
				break;
				case Track::AUDIO_INPUT:
					for (int channel = 0; channel < track->channels(); ++channel)
					{
						newDstList->addItem(Route(track, channel).name());
					}
					newSrcList->addItem(Route(track, -1).name());
				break;
				default:
					newDstList->addItem(Route(track, -1).name());
					newSrcList->addItem(Route(track, -1).name());
				break;/*}}}*/
			}
		}

		const RouteList* rl = atrack->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			QString src(atrack->name());
			if (atrack->type() == Track::AUDIO_OUTPUT)
			{
				Route s(src, false, r->channel);
				src = s.name();
			}
			new QTreeWidgetItem(routeList, QStringList() << src << r->name());
		}
		if (checkAudioDevice()) 
		{
			std::list<QString> sl = audioDevice->outputPorts();
			for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
				newSrcList->addItem(*i);
			}
			if(atrack->type() != Track::AUDIO_AUX)
			{
				sl = audioDevice->inputPorts();
				for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
					newDstList->addItem(*i);
				}
			}
		}
		routeSelectionChanged(); // init remove button
		srcSelectionChanged(); // init select button
	}
}

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
{
	QListWidgetItem* srcItem = newSrcList->currentItem();
	//QListWidgetItem* dstItem = newDstList->currentItem();
	if(srcItem)
	{
		QList<QTreeWidgetItem*> found = routeList->findItems(srcItem->text(), Qt::MatchExactly, 0);
		connectButton->setEnabled(found.isEmpty());//(srcItem != 0) && (dstItem != 0) && checkRoute(srcItem->text(), dstItem->text()));
	}
	else
		connectButton->setEnabled(false);
}

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
{
	QListWidgetItem* dstItem = newDstList->currentItem();
	//QListWidgetItem* srcItem = newSrcList->currentItem();
	if(dstItem)
	{
		QList<QTreeWidgetItem*> found = routeList->findItems(dstItem->text(), Qt::MatchExactly, 1);
		btnConnectOut->setEnabled(found.isEmpty());
	}
	else
		btnConnectOut->setEnabled(false);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
{
	emit closed();
	e->accept();
}
