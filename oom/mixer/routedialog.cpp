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
	_selected = 0;
	selectedIndex = -1;
	connect(routeList, SIGNAL(itemSelectionChanged()), SLOT(routeSelectionChanged()));
	connect(newSrcList, SIGNAL(itemSelectionChanged()), SLOT(srcSelectionChanged()));
	connect(newDstList, SIGNAL(itemSelectionChanged()), SLOT(dstSelectionChanged()));
	connect(tracksList, SIGNAL(itemSelectionChanged()), SLOT(trackSelectionChanged()));
	connect(removeButton, SIGNAL(clicked()), SLOT(removeRoute()));
	connect(connectButton, SIGNAL(clicked()), SLOT(addRoute()));
	connect(btnConnectOut, SIGNAL(clicked()), SLOT(addOutRoute()));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	routingChanged();
	//QTreeWidgetItem* header = routeList->headerItem();
	//if(header)
	//{
		routeList->header()->resizeSection(1, 30);
		routeList->header()->resizeSection(3, 30);
	//}
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
		AudioTrack* track = (AudioTrack*) (*i);
		//if (track->type() == Track::AUDIO_OUTPUT)
		//{
			for (int channel = 0; channel < track->channels(); ++channel)
			{
				Route r(track, channel);
				tracksList->addItem(r.name());
			}
		//}
		//else
		//	tracksList->addItem(Route(track, -1).name());
	}
	if(selectedIndex < tracksList->count())
		tracksList->setCurrentRow(selectedIndex, QItemSelectionModel::ClearAndSelect);
	//if(_selected)
	//	setSelected(_selected->name());
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void RouteDialog::songChanged(int v)
{
	if (v & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_ROUTE | SC_CHANNELS))
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
	Route srcRoute(item->text(0), true, item->text(1).toInt());
	Route dstRoute(item->text(2), true, item->text(3).toInt());
	audio->msgRemoveRoute(srcRoute, dstRoute);
	//audio->msgRemoveRoute(Route(item->text(0), false, -1), Route(item->text(1), true, -1));
	audio->msgUpdateSoloStates();
	//song->update(SC_SOLO);
	song->update(SC_ROUTE);
	//delete item;
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
		selectedIndex = tracksList->row(titem);
		//TrackList* tl = song->tracks();
		//for(iTrack t = tl->begin(); t != tl->end(); ++t)
		//{
		//	if((*t)->isMidiTrack())
		//		continue;
		//	AudioTrack* track = (AudioTrack*) (*t);
		//	if(track->name() == atrack->name())
		//		continue; //You cant connect a track to itself
			//int channels = track->channels();
			switch (atrack->type())
			{
				case Track::AUDIO_OUTPUT:/*{{{*/
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name() || track->type() == Track::AUDIO_OUTPUT)
							continue; //You cant connect a track to itself
						for (int channel = 0; channel < track->channels(); ++channel)
						{
							Route r(track, channel);
							newSrcList->addItem(r.name());
						}
					}
					insertInputs();
					//newDstList->addItem(Route(track, -1).name());
				break;
				case Track::AUDIO_AUX:
					newSrcList->clear();
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name())
							continue; //You cant connect a track to itself
						if(track->type() ==  Track::AUDIO_BUSS || track->type() == Track::AUDIO_OUTPUT)
							newDstList->addItem(Route(track, -1).name());
						//newDstList->addItem(Route(track, -1).name());
					}
				break;
				case Track::AUDIO_INPUT:
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name())
							continue; //You cant connect a track to itself
						switch(track->type())
						{
							case Track::AUDIO_OUTPUT:
							case Track::AUDIO_BUSS:
							case Track::WAVE:
								for (int channel = 0; channel < track->channels(); ++channel)
								{
									newDstList->addItem(Route(track, channel).name());
								}
							break;
							default:
							break;
						}
					}
					insertOutputs();
					//newSrcList->addItem(Route(track, -1).name());
				break;
				case Track::WAVE:
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name())
							continue; //You cant connect a track to itself
						if(track->type() == Track::AUDIO_INPUT)
						{
							for (int channel = 0; channel < track->channels(); ++channel)
							{
								newSrcList->addItem(Route(track, channel).name());
							}
						}
						else if(track->type() == Track::AUDIO_OUTPUT || track->type() == Track::AUDIO_BUSS)
						{
							for (int channel = 0; channel < track->channels(); ++channel)
							{
								newDstList->addItem(Route(track, channel).name());
							}
						}
					}
				break;
				case Track::AUDIO_BUSS:
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name())
							continue; //You cant connect a track to itself
						if(track->type() == Track::AUDIO_INPUT || track->type() == Track::WAVE || track->type() == Track::AUDIO_SOFTSYNTH)
						{
							for (int channel = 0; channel < track->channels(); ++channel)
							{
								newSrcList->addItem(Route(track, channel).name());
							}
						}
						else if(track->type() == Track::AUDIO_OUTPUT)
						{
							for (int channel = 0; channel < track->channels(); ++channel)
							{
								newDstList->addItem(Route(track, channel).name());
							}
						}
					}
				break;
				case Track::AUDIO_SOFTSYNTH:
					newSrcList->clear();
					for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
					{
						if((*t)->isMidiTrack())
							continue;
						AudioTrack* track = (AudioTrack*) (*t);
						if(track->name() == atrack->name())
							continue; //You cant connect a track to itself
						if(track->type() == Track::AUDIO_OUTPUT || track->type() == Track::AUDIO_BUSS)
						{
							for (int channel = 0; channel < track->channels(); ++channel)
							{
								newDstList->addItem(Route(track, channel).name());
							}
						}
					}
				break;
				default:
					//newDstList->addItem(Route(track, -1).name());
					//newSrcList->addItem(Route(track, -1).name());
				break;/*}}}*/
			}
		//}

		const RouteList* rl = atrack->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			QString src(atrack->name());
			if (atrack->type() == Track::AUDIO_OUTPUT)
			{
				Route s(src, false, r->channel);
				src = s.name();
				new QTreeWidgetItem(routeList, QStringList() << src << QString::number(s.channel) << r->name() << QString::number(r->channel));
			}
			else
			{
				new QTreeWidgetItem(routeList, QStringList() << src << QString::number(0) << r->name() << QString::number(0));
			}
		}
		const RouteList* rli = atrack->inRoutes();
		for (ciRoute ri = rli->begin(); ri != rli->end(); ++ri)
		{
			QString src(atrack->name());
			if (atrack->type() == Track::AUDIO_INPUT)
			{
				Route s(src, false, ri->channel);
				src = s.name();
				new QTreeWidgetItem(routeList, QStringList() << ri->name() << QString::number(ri->channel) << src << QString::number(s.channel));
			}
			else
			{
				new QTreeWidgetItem(routeList, QStringList() << ri->name() << QString::number(0) << src << QString::number(0));
			}
		}
		routeSelectionChanged(); // init remove button
		srcSelectionChanged(); // init select button
	}
}

void RouteDialog::insertOutputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			newSrcList->addItem(*i);
		}
	}
}

void RouteDialog::insertInputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			newDstList->addItem(*i);
		}
	}
}

void RouteDialog::setSelected(QString t)
{
	QList<QListWidgetItem*> found = tracksList->findItems(t, Qt::MatchExactly);
	if(found.isEmpty())
		return;
	tracksList->setCurrentItem(found.at(0));
}

void RouteDialog::setSelected(AudioTrack* t)
{
	//_selected = s;
	if(t)
	{
		QList<QListWidgetItem*> found = tracksList->findItems(t->name(), Qt::MatchExactly);
		if(found.isEmpty())
			return;
		tracksList->setCurrentItem(found.at(0));
	}
}


//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
{
	QListWidgetItem* srcItem = newSrcList->currentItem();
	QListWidgetItem* tItem = tracksList->currentItem();
	if(srcItem)
	{
		int chan = 0;
		if(!tItem)
			return;
		int row = tracksList->row(tItem);
		QList<QListWidgetItem*> tfound = tracksList->findItems(tItem->text(), Qt::MatchExactly);
		if(tfound.isEmpty())
			return;
		for(int i = 0; i < tfound.size(); ++i)
		{
			QListWidgetItem* item = tfound.at(i);
			chan = i;
			int r = tracksList->row(item);
			if(r == row)
				break;
		}
		QList<QTreeWidgetItem*> found = routeList->findItems(srcItem->text(), Qt::MatchExactly, 0);
		bool en = true;
		if(!found.isEmpty())
		{
			for(int i = 0; i < found.size(); ++i)
			{
				QTreeWidgetItem* r = found.at(i);
				if(r->text(1).toInt() == chan)
				{
					en = false;
					break;
				}
			}
		}
		connectButton->setEnabled(en);//(srcItem != 0) && (dstItem != 0) && checkRoute(srcItem->text(), dstItem->text()));
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
	QListWidgetItem* tItem = tracksList->currentItem();
	if(dstItem)
	{
		int chan = 0;
		if(!tItem)
			return;
		int row = tracksList->row(tItem);
		QList<QListWidgetItem*> tfound = tracksList->findItems(tItem->text(), Qt::MatchExactly);
		if(tfound.isEmpty())
			return;
		for(int i = 0; i < tfound.size(); ++i)
		{
			QListWidgetItem* item = tfound.at(i);
			chan = i;
			int r = tracksList->row(item);
			if(r == row)
				break;
		}
		QList<QTreeWidgetItem*> found = routeList->findItems(dstItem->text(), Qt::MatchExactly, 3);
		bool en = true;
		if(!found.isEmpty())
		{
			for(int i = 0; i < found.size(); ++i)
			{
				QTreeWidgetItem* r = found.at(i);
				if(r->text(1).toInt() == chan)
				{
					en = false;
					break;
				}
			}
		}
		btnConnectOut->setEnabled(en);
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
