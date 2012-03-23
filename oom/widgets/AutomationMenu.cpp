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
	m_synthTrack = 0;
	m_inputTrack = 0;
	m_controllers = 0;
	list = 0;
	m_inputList = 0;
	m_inputControllers = 0;
}

QWidget* AutomationMenu::createWidget(QWidget* parent)
{
	if(!m_track)
		return 0;

	int baseHeight = 109;
	int desktopHeight = qApp->desktop()->height();
	int lstr = 0;
	QString longest;
    
	QVBoxLayout* layout = new QVBoxLayout();
	QWidget* w = new QWidget(parent);
	w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QLabel* header = new QLabel();
	header->setPixmap(QPixmap(":/images/automation_menu_title.png"));
	header->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	header->setObjectName("AutomationMenuHeader");
	QLabel* tvname = new QLabel(m_track->name());
	tvname->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tvname->setObjectName("AutomationMenuLabel");
	layout->addWidget(header);
	layout->addWidget(tvname);
	m_listModel = new QStandardItemModel();
	m_inputListModel = new QStandardItemModel();
    if (m_track->isMidiTrack())
    {
		m_inputTrack = m_track->inputTrack();
		if(m_inputTrack)/*{{{*/
		{//build the input list and set the input controller
			m_inputList = new QListView();
			m_inputList->setObjectName("AutomationMenuList");
			m_inputList->setSelectionMode(QAbstractItemView::SingleSelection);
			m_inputList->setAlternatingRowColors(true);
			m_inputList->setEditTriggers(QAbstractItemView::NoEditTriggers);
			m_inputList->setModel(m_inputListModel);
			m_inputList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			m_inputList->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
			m_inputList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

			layout->addWidget(m_inputList);
			m_inputControllers = ((AudioTrack*)m_inputTrack)->controller();
		}/*}}}*/
		if(m_track->wantsAutomation())
		{
       		MidiPort* mp = &midiPorts[((MidiTrack*) m_track)->outPort()];
			if (mp->device() && mp->device()->isSynthPlugin())
			{
				SynthPluginDevice* synth = (SynthPluginDevice*)mp->device();
				if (synth->audioTrack())
				{
					m_controllers = synth->audioTrack()->controller();
					m_synthTrack = synth->audioTrack();
				}
				else
					return 0;
			}
			else
				return 0;
			list = new QListView();
			list->setObjectName("AutomationMenuList");
			list->setSelectionMode(QAbstractItemView::SingleSelection);
			list->setAlternatingRowColors(true);
			list->setEditTriggers(QAbstractItemView::NoEditTriggers);
			list->setModel(m_listModel);
			list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
			list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			layout->addWidget(list);

		}
    }
    else
    {
		list = new QListView();
		list->setObjectName("AutomationMenuList");
		list->setSelectionMode(QAbstractItemView::SingleSelection);
		list->setAlternatingRowColors(true);
		list->setEditTriggers(QAbstractItemView::NoEditTriggers);
		list->setModel(m_listModel);
		list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		layout->addWidget(list);
        m_controllers = ((AudioTrack*) m_track)->controller();
    }

	int inputCount = 0;
	int synthCount = 0;
	if(m_inputTrack)
	{
		for (CtrlListList::iterator icll = m_inputControllers->begin(); icll != m_inputControllers->end(); ++icll)/*{{{*/
    	{
    	    CtrlList *cl = icll->second;
    	    if (cl->dontShow())
    	        continue;
    	    QString name(cl->pluginName().isEmpty() ? cl->name() : cl->pluginName() + " : " + cl->name()); 
			if(name.isEmpty())
				continue; //I am seeing ports with no names show up, lets avoid these as they may cause us problems later
			
			baseHeight += 18;
			inputCount++;
			
			if(name.length() > lstr)
			{
				lstr = name.length();
				longest = name;
			}
			QStandardItem* item = new QStandardItem(name);
    	    item->setCheckable(true);
    	    item->setCheckState(cl->isVisible() ? Qt::Checked : Qt::Unchecked);
    	    item->setData(cl->id());
			if(cl->size() > 1)
				item->setForeground(cl->color());
			m_inputListModel->appendRow(item);
    	}/*}}}*/
		connect(m_inputList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(inputItemClicked(const QModelIndex&)));
	}
	w->setLayout(layout);
	
	if(list)
	{
		for (CtrlListList::iterator icll = m_controllers->begin(); icll != m_controllers->end(); ++icll)/*{{{*/
	    {
	        CtrlList *cl = icll->second;
	        if (cl->dontShow())
	            continue;
	        QString name(cl->pluginName().isEmpty() ? cl->name() : cl->pluginName() + " : " + cl->name()); 
			if(name.isEmpty())
				continue; //I am seeing ports with no names show up, lets avoid these as they may cause us problems later

			baseHeight += 18;
			synthCount++;

			if(name.length() > lstr)
			{
				lstr = name.length();
				longest = name;
			}
			QStandardItem* item = new QStandardItem(name);
	        item->setCheckable(true);
	        item->setCheckState(cl->isVisible() ? Qt::Checked : Qt::Unchecked);
	        item->setData(cl->id());
			if(cl->size() > 1)
				item->setForeground(cl->color());
			m_listModel->appendRow(item);
	    }/*}}}*/
		connect(list, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)));
		if(m_inputTrack)
		{
			if(inputCount > synthCount)
			{
				int factor = inputCount / synthCount;
				layout->setStretch(2, factor);
			}
			else
			{
				int factor = synthCount / inputCount;
				layout->setStretch(3, factor);
			}
		}
	}
	
	if(baseHeight > desktopHeight)
		baseHeight = (desktopHeight-50);
	w->setFixedHeight(baseHeight);
	QFontMetrics fm(list ? list->font() : m_inputList->font());
	QRect rect = fm.boundingRect(longest);
	int width = rect.width()+100;
	if(width < 170)
		width = 170;
	w->setFixedWidth(width);
	//connect(m_listModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateData(QStandardItem*)));
	return w;
}

void AutomationMenu::inputItemClicked(const QModelIndex& index)/*{{{*/
{
	itemClicked(index, true);
}/*}}}*/

void AutomationMenu::itemClicked(const QModelIndex& index, bool isInput)/*{{{*/
{
	if((!isInput && !m_controllers) || (isInput && !m_inputControllers) || !m_track)
		return;
	if(index.isValid())
	{
		QStandardItem *item = 0;
		if(isInput)
			item = m_inputListModel->item(index.row());
		else
			item = m_listModel->item(index.row());
		if(item)
		{
			if(isInput)
			{
				int id = item->data().toInt();/*{{{*/
				iCtrlList it = m_inputControllers->find(id);
				if(it != m_inputControllers->end())
				{
					CtrlList* cl = it->second;
					bool checked = (item->checkState() == Qt::Checked);
					if(checked == cl->isVisible())
					{
						item->setCheckState( checked ? Qt::Unchecked : Qt::Checked);
					}
					cl->setVisible(!cl->isVisible());
					if(cl->id() == AC_PAN)
					{
						AutomationType at = ((AudioTrack*)m_inputTrack)->automationType();
						if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
							((AudioTrack*)m_inputTrack)->enablePanController(false);
					
						double panVal = ((AudioTrack*)m_inputTrack)->pan();
						audio->msgSetPan(((AudioTrack*)m_inputTrack), panVal);
						((AudioTrack*)m_inputTrack)->startAutoRecord(AC_PAN, panVal);

						if (((AudioTrack*)m_inputTrack)->automationType() != AUTO_WRITE)
							((AudioTrack*)m_inputTrack)->enablePanController(true);
						((AudioTrack*)m_inputTrack)->stopAutoRecord(AC_PAN, panVal);
					}
					song->update(SC_TRACK_MODIFIED);
				}/*}}}*/
			}
			else
			{
				int id = item->data().toInt();/*{{{*/
				iCtrlList it = m_controllers->find(id);
				if(it != m_controllers->end())
				{
					CtrlList* cl = it->second;
					bool checked = (item->checkState() == Qt::Checked);
					if(checked == cl->isVisible())
					{
						item->setCheckState( checked ? Qt::Unchecked : Qt::Checked);
					}
					cl->setVisible(!cl->isVisible());
					if(cl->id() == AC_PAN)
					{
						AutomationType at = ((AudioTrack*)m_track)->automationType();
						if (at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
							((AudioTrack*)m_track)->enablePanController(false);
					
						double panVal = ((AudioTrack*)m_track)->pan();
						audio->msgSetPan(((AudioTrack*)m_track), panVal);
						((AudioTrack*)m_track)->startAutoRecord(AC_PAN, panVal);

						if (((AudioTrack*)m_track)->automationType() != AUTO_WRITE)
							((AudioTrack*)m_track)->enablePanController(true);
						((AudioTrack*)m_track)->stopAutoRecord(AC_PAN, panVal);
					}
					song->update(SC_TRACK_MODIFIED);
					//updateData(item);
				}/*}}}*/
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
}/*}}}*/
