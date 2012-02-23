//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#include "AutomationMenu.h"

#include <QtGui>

#include "track.h"
#include "globals.h"
#include "icons.h"
#include "song.h"
#include "node.h"
#include "audio.h"
#include "app.h"
#include "gconfig.h"
#include "config.h"
#include "midiport.h"
#include "plugin.h"
#include "mididev.h"

AutomationMenu::AutomationMenu(QMenu* parent, Track *t)
: QWidgetAction(parent)
{
	m_track= t;
}

QWidget* AutomationMenu::createWidget(QWidget* parent)
{
	if(!m_track || (m_track->isMidiTrack() && !m_track->wantsAutomation()))
		return 0;

	int baseHeight = 40;
	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	QLabel* tvname = new QLabel(m_track->name());
	tvname->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tvname->setObjectName("AutomationMenuLabel");
	layout->addWidget(tvname);
	m_listModel = new QStandardItemModel();
	list = new QListView();
	list->setObjectName("AutomationMenuList");
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setAlternatingRowColors(true);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	list->setModel(m_listModel);
	layout->addWidget(list);
	w->setLayout(layout);

    CtrlListList* cll;
    if (m_track->isMidiTrack() && m_track->wantsAutomation())
    {
        MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
        if (mp->device() && mp->device()->isSynthPlugin())
        {
            SynthPluginDevice* synth = (SynthPluginDevice*)mp->device();
            if (synth->audioTrack())
                cll = synth->audioTrack()->controller();
            else
                return 0;
        }
        else
            return 0;
    }
    else
    {
        cll = ((AudioTrack*) m_track)->controller();
    }
	int size = cll->size();
	baseHeight += 20*size;
	int desktopHeight = qApp->desktop()->height();
	if(baseHeight > desktopHeight)
		baseHeight = (desktopHeight-50);
	w->setFixedHeight(baseHeight);
    for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
    {
        CtrlList *cl = icll->second;
        if (cl->dontShow())
            continue;
        QString name(cl->pluginName().isEmpty() ? cl->name() : cl->pluginName() + " : " + cl->name()); 
		QStandardItem* item = new QStandardItem(name);
        item->setCheckable(true);
        item->setCheckState(cl->isVisible() ? Qt::Checked : Qt::Unchecked);
        item->setData(cl->id());
		m_listModel->appendRow(item);
    }
	connect(m_listModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateData(QStandardItem*)));
	return w;
}

void AutomationMenu::updateData(QStandardItem *item)
{
	if(list && item)
	{
		int id = item->data().toInt();
		AudioTrack* atrack = 0;
    	if (m_track->isMidiTrack())
    	{
    	    MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
    	    if (mp->device() && mp->device()->isSynthPlugin())
    	    {
    	        SynthPluginDevice* synth = (SynthPluginDevice*)mp->device();
    	        if (synth->audioTrack())
    	            atrack = (AudioTrack*)synth->audioTrack();
    	        else
    	            return;
    	    }
    	    else
    	        return;
    	}
		else
		{
			atrack = (AudioTrack*)m_track;
		}

		if(atrack)
		{
			CtrlListList* cll = atrack->controller();
			for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
			{
				CtrlList *cl = icll->second;
				if (id == cl->id()) 
				{
					cl->setVisible(!cl->isVisible());
					if(cl->id() == AC_PAN)
					{
						AutomationType at = atrack->automationType();
						if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
							atrack->enablePanController(false);
					
						double panVal = atrack->pan();
						audio->msgSetPan(atrack, panVal);
						atrack->startAutoRecord(AC_PAN, panVal);

						if (atrack->automationType() != AUTO_WRITE)
							atrack->enablePanController(true);
						atrack->stopAutoRecord(AC_PAN, panVal);
					}
					song->update(SC_TRACK_MODIFIED);
					break;
				}
			}
		}
		//printf("Triggering Menu Action\n");
		//setData(item->data(Qt::UserRole+3));
		
		//FIXME: This is a seriously brutal HACK but its the only way I can get it to dismiss the menu
		/*QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e);

		QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e2);*/
	}
}
