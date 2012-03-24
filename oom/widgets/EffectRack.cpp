//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  $Id: rack.cpp,v 1.7.2.7 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <QByteArray>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QUrl>

#include <errno.h>

#include "xml.h"
#include "EffectRack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "globals.h"
#include "plugin.h"
#include "filedialog.h"
#include "plugindialog.h"
#include "plugingui.h"

//---------------------------------------------------------
//   class RackSlot
//---------------------------------------------------------

class RackSlot : public QListWidgetItem
{
	int idx;
	AudioTrack* node;

public:
	RackSlot(QListWidget* lb, AudioTrack* t, int i);
	virtual ~RackSlot();

	void setBackgroundColor(const QBrush& brush)
	{
		setBackground(brush);
	}
};

RackSlot::~RackSlot()
{
	node = 0;
}

//---------------------------------------------------------
//   RackSlot
//---------------------------------------------------------

RackSlot::RackSlot(QListWidget* b, AudioTrack* t, int i)
: QListWidgetItem(b)
{
	node = t;
	idx = i;
	setSizeHint(QSize(10, 17));
}

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, AudioTrack* t)
: QListWidget(parent)
{
	setObjectName("Rack");
	setAttribute(Qt::WA_DeleteOnClose);
	track = t;
	setFont(config.fonts[1]);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSelectionMode(QAbstractItemView::SingleSelection);
	for (int i = 0; i < PipelineDepth; ++i)
		new RackSlot(this, track, i);
	updateContents();

	connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
			this, SLOT(doubleClicked(QListWidgetItem*)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
    connect(song, SIGNAL(segmentSizeChanged(int)), SLOT(segmentSizeChanged(int)));

	setSpacing(0);

	setAcceptDrops(true);
}

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::~EffectRack()
{
	//qDebug("Deleting Effect Rack");
}

void EffectRack::updateContents()
{
    if (!track)
        return;
	Pipeline* pipeline = track->efxPipe();
	int pdepth = 0;
	if(pipeline)
		pdepth = pipeline->size();
	if(pdepth == 0)
	{
		for (int i = 0; i < PipelineDepth; ++i)
		{
			QString name("empty");
			item(i)->setText(name);
			item(i)->setToolTip(tr("effect rack"));
		}
	}
	else
	{
		for (int i = 0; i < PipelineDepth; ++i)
		{
			QString name("empty");
			item(i)->setToolTip(tr("effect rack"));
			if(i < pdepth)
			{
				name = pipeline->name(i);
				item(i)->setToolTip(name);
			}
			item(i)->setText(name);
		}
	}
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void EffectRack::songChanged(int typ)
{
	if (typ & (SC_ROUTE | SC_RACK) || typ == -1)
	{
		updateContents();
	}
}

//---------------------------------------------------------
//   segmentSizeChanged
//---------------------------------------------------------

void EffectRack::segmentSizeChanged(int size)
{
	Q_UNUSED(size);
    /*Pipeline* pipe = track->efxPipe();
	int pdepth = pipe->size();
    for (int i = 0; i < pdepth; ++i)
    {
        if ((*pipe)[i])
        {
            // FIXME - this needs to happen on the same call as jack-buffer-size change callback
            //(*pipe)[i]->bufferSizeChanged(size);
        }
    }*/
}

void EffectRack::choosePlugin(QListWidgetItem* it, bool replace)/*{{{*/
{
    PluginI* plugi = PluginDialog::getPlugin(track->type(), this);
    if (plugi)
    {
        BasePlugin* nplug = 0;

        if (plugi->type() == PLUGIN_LADSPA)
            nplug = new LadspaPlugin();
        else if (plugi->type() == PLUGIN_LV2)
            nplug = new Lv2Plugin();
        else if (plugi->type() == PLUGIN_VST)
            nplug = new VstPlugin();
        
        if (nplug)
        {
            if (nplug->init(plugi->filename(), plugi->label()))
            {
                // just in case is needed later
                //if (!audioDevice || audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
                //    nplug->aboutToRemove();

                int idx = row(it);
                if (replace)
                {
                    audio->msgAddPlugin(track, idx, 0);
                    //Do this part from the GUI context so user interfaces can be properly deleted
                    // track->efxPipe()->insert(0, idx); was set on lv2 only
                }
                audio->msgAddPlugin(track, idx, nplug);
                nplug->setChannels(track->channels());
                nplug->setActive(true);
                song->dirty = true;
            }
            else
            {
                QMessageBox::warning(this, tr("Failed to load plugin"), tr("Plugin '%1'' failed to initialize properly, error was:\n%2").arg(plugi->name()).arg(get_last_error()));
                nplug->deleteMe();
                return;
            }
        }

        updateContents();
    }
}/*}}}*/

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::menuRequested(QListWidgetItem* it)/*{{{*/
{
	if (it == 0 || track == 0)
		return;
	RackSlot* curitem = (RackSlot*) it;

	Pipeline* epipe = track->efxPipe();
	
	int idx = row(curitem);
	QString name;
	bool mute = false;
    bool nativeGui = false;
	Pipeline* pipe = track->efxPipe();
	if (pipe)
	{
		name = pipe->name(idx);
		mute = (pipe->isActive(idx) == false);
        nativeGui = pipe->hasNativeGui(idx);
	}

	//enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SAVE };

	enum
	{
		NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SHOW_NATIVE, SAVE
	};
	QMenu* menu = new QMenu;
	QAction* newAction = menu->addAction(tr("new"));
	QAction* changeAction = menu->addAction(tr("change"));
	QAction* upAction = menu->addAction(QIcon(*upIcon), tr("move up")); //,   UP, UP);
	QAction* downAction = menu->addAction(QIcon(*downIcon), tr("move down")); //, DOWN, DOWN);
	QAction* removeAction = menu->addAction(tr("remove")); //,    REMOVE, REMOVE);
	QAction* bypassAction = menu->addAction(tr("bypass")); //,    BYPASS, BYPASS);
	QAction* showGuiAction = menu->addAction(tr("show gui")); //,  SHOW, SHOW);
	QAction* showNativeGuiAction = menu->addAction(tr("show native gui")); //,  SHOW_NATIVE, SHOW_NATIVE);
	QAction* saveAction = menu->addAction(tr("save preset"));

	newAction->setData(NEW);
	changeAction->setData(CHANGE);
	upAction->setData(UP);
	downAction->setData(DOWN);
	removeAction->setData(REMOVE);
	bypassAction->setData(BYPASS);
	showGuiAction->setData(SHOW);
	showNativeGuiAction->setData(SHOW_NATIVE);
	saveAction->setData(SAVE);

	bypassAction->setCheckable(true);
	showGuiAction->setCheckable(true);
	showNativeGuiAction->setCheckable(true);

	bypassAction->setChecked(mute);
	showGuiAction->setChecked(pipe->guiVisible(idx));
    showNativeGuiAction->setEnabled(nativeGui);
    if (nativeGui)
        showNativeGuiAction->setChecked(pipe->nativeGuiVisible(idx));

	if (pipe->empty(idx))
	{
		menu->removeAction(changeAction);
		menu->removeAction(saveAction);
		upAction->setEnabled(false);
		downAction->setEnabled(false);
		removeAction->setEnabled(false);
		bypassAction->setEnabled(false);
		showGuiAction->setEnabled(false);
		showNativeGuiAction->setEnabled(false);
	}
	else
	{
		menu->removeAction(newAction);
		if (idx == 0)
			upAction->setEnabled(true);
		if (idx == ((int)epipe->size() - 1))
			downAction->setEnabled(false);
	}

	QPoint pt = QCursor::pos();
	QAction* act = menu->exec(pt, 0);

	//delete menu;
	if (!act)
	{
		delete menu;
		return;
	}

	int sel = act->data().toInt();
	delete menu;

	int pdepth = epipe->size();
	switch (sel)
	{
		case NEW:
		{
			choosePlugin(it);
			break;
		}
		case CHANGE:
		{
			choosePlugin(it, true);
			break;
		}
		case REMOVE:
		{
            BasePlugin* oldPlugin = (*epipe)[idx];
            oldPlugin->setActive(false);
            oldPlugin->aboutToRemove();

            if(debugMsg)
				qCritical("Plugin to remove now and here");

            audio->msgAddPlugin(track, idx, 0);
			song->dirty = true;
			break;
		}
		case BYPASS:
		{
			bool flag = !pipe->isActive(idx);
			pipe->setActive(idx, flag);
			break;
		}
		case SHOW:
		{
			bool flag = !pipe->guiVisible(idx);
			pipe->showGui(idx, flag);
			break;
		}
		case SHOW_NATIVE:
		{
			printf("Show native GUI called\n");
			bool flag = !pipe->nativeGuiVisible(idx);
			pipe->showNativeGui(idx, flag);
			break;
		}
		case UP:
			if (idx > 0)
			{
				setCurrentItem(item(idx - 1));
				pipe->move(idx, true);
			}
			break;
		case DOWN:
			if (idx < pdepth)
			{
				setCurrentItem(item(idx + 1));
				pipe->move(idx, false);
			}
			break;
		case SAVE:
			savePreset(idx);
			break;
	}
	//Already done on songChanged
	//updateContents();
	song->update(SC_RACK);
}/*}}}*/

//---------------------------------------------------------
//   doubleClicked
//    toggle gui
//---------------------------------------------------------

void EffectRack::doubleClicked(QListWidgetItem* it)
{
	if (it == 0 || track == 0)
		return;

	RackSlot* item = (RackSlot*) it;
	int idx = row(item);
	Pipeline* pipe = track->efxPipe();

	if (pipe->name(idx) == QString("empty"))
	{
		choosePlugin(it);
		return;
	}

    if (pipe)
    {
        if (pipe->hasNativeGui(idx))
        {
            bool flag = !pipe->nativeGuiVisible(idx);
            pipe->showNativeGui(idx, flag);
        }
        else
        {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
        }
    }
}

void EffectRack::savePreset(int idx)/*{{{*/
{
	//QString name = getSaveFileName(QString(""), plug_file_pattern, this,
	QString name = getSaveFileName(QString(""), preset_file_save_pattern, this,
			tr("OOStudio: Save Preset"));

	if (name.isEmpty())
		return;

	//FILE* presetFp = fopen(name.ascii(),"w+");
	bool popenFlag;
	FILE* presetFp = fileOpen(this, name, QString(".pre"), "w", popenFlag, false, true);
	if (presetFp == 0)
	{
		//fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n",
		//   strerror(errno));
		return;
	}
	Xml xml(presetFp);
	Pipeline* pipe = track->efxPipe();
	if (pipe)
	{
		if ((*pipe)[idx] != NULL)
		{
			xml.header();
			xml.tag(0, "oom version=\"2.0\"");
			(*pipe)[idx]->writeConfiguration(1, xml);
			xml.tag(0, "/oom");
		}
		else
		{
			printf("no plugin!\n");
			//fclose(presetFp);
			if (popenFlag)
				pclose(presetFp);
			else
				fclose(presetFp);
			return;
		}
	}
	else
	{
		printf("no pipe!\n");
		//fclose(presetFp);
		if (popenFlag)
			pclose(presetFp);
		else
			fclose(presetFp);
		return;
	}
	//fclose(presetFp);
	if (popenFlag)
		pclose(presetFp);
	else
		fclose(presetFp);
}/*}}}*/

void EffectRack::startDrag(int idx)
{
	FILE* tmp = tmpfile();
	if (tmp == 0)
	{
		fprintf(stderr, "EffectRack::startDrag fopen failed: %s\n",
				strerror(errno));
		return;
	}
	Xml xml(tmp);
	Pipeline* pipe = track->efxPipe();
	if (pipe)
	{
		int size = pipe->size();
		if (idx < size)
		{
			xml.header();
			xml.tag(0, "oom version=\"2.0\"");
			(*pipe)[idx]->writeConfiguration(1, xml);
			xml.tag(0, "/oom");
		}
		else
		{
			//printf("no plugin!\n");
			return;
		}
	}
	else
	{
		//printf("no pipe!\n");
		return;
	}

	QString xmlconf;
	xml.dump(xmlconf);

	QByteArray data(xmlconf.toLatin1().constData());
	QString strData(data);
	//qDebug("EffectRack::startDrag: Generated Drag Copy data:\n%s",strData.toUtf8().constData());
	QMimeData* md = new QMimeData();

	md->setData("text/x-oom-plugin", data);

	QDrag* drag = new QDrag(this);
	drag->setMimeData(md);

	drag->exec(Qt::CopyAction);
}

Qt::DropActions EffectRack::supportedDropActions() const
{
	return Qt::CopyAction;
}

QStringList EffectRack::mimeTypes() const
{
	return QStringList("text/x-oom-plugin");
}

void EffectRack::dropEvent(QDropEvent *event)/*{{{*/
{
    event->accept();
	QString text;
	QListWidgetItem *i = itemAt(event->pos());
	if (!i)
		return;
	int idx = row(i);
	//qDebug("EffectRack::dropEvent: idx: %d", idx);

	Pipeline* pipe = track->efxPipe();
	if (pipe)
	{
		//int size = pipe->size();
		/*if (idx < size)
		{
			QWidget *sw = event->source();
			if (sw)
			{
				if (strcmp(sw->metaObject()->className(), "EffectRack") == 0)
				{
					EffectRack *ser = (EffectRack*) sw;
					Pipeline* spipe = ser->getTrack()->efxPipe();
					if (!spipe)
						return;

					QListWidgetItem *i = ser->itemAt(ser->getDragPos());
					int idx0 = ser->row(i);
					if (!(*spipe)[idx0] ||
							(idx == idx0 && (ser == this || ser->getTrack()->name() == track->name())))
						return;
				}
			}
			if (QMessageBox::question(this, tr("Replace effect"), tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
			{
				audio->msgAddPlugin(track, idx, 0);
				song->update(SC_RACK);
			}
			else
			{
				return;
			}
		}*/

		if (event->mimeData()->hasFormat("text/x-oom-plugin"))
		{
			const QMimeData *md = event->mimeData();
			QString outxml(md->data("text/x-oom-plugin"));
			//qDebug("EffectRack::dropEvent Event data:\n%s", outxml.toUtf8().constData());
			//Xml xml(event->mimeData()->data("text/x-oom-plugin").data());
			QByteArray ba = outxml.toUtf8();
			const char* data =  ba.constData();
			Xml xml(data);
			initPlugin(xml, idx);
		}
		else if (event->mimeData()->hasUrls())
		{
			// Multiple urls not supported here. Grab the first one.
			text = event->mimeData()->urls()[0].path();

			if (text.endsWith(".pre", Qt::CaseInsensitive) ||
					text.endsWith(".pre.gz", Qt::CaseInsensitive) ||
					text.endsWith(".pre.bz2", Qt::CaseInsensitive))
			{
				//bool popenFlag = false;
				bool popenFlag;
				FILE* fp = fileOpen(this, text, ".pre", "r", popenFlag, false, false);
				if (fp)
				{
					Xml xml(fp);
					initPlugin(xml, idx);

					if (popenFlag)
						pclose(fp);
					else
						fclose(fp);
				}
			}
		}
	}
}/*}}}*/

void EffectRack::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void EffectRack::mousePressEvent(QMouseEvent *event)
{
	if (event->button() & Qt::LeftButton)
	{
		dragPos = event->pos();
	}
	else if (event->button() & Qt::RightButton)
	{
		menuRequested(itemAt(event->pos()));
		return;
	}
	else if (event->button() & Qt::MidButton)
	{
		int idx = row(itemAt(event->pos()));
		bool flag = !track->efxPipe()->isActive(idx);
		track->efxPipe()->setActive(idx, flag);
		updateContents();
	}

	QListWidget::mousePressEvent(event);
}

void EffectRack::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		Pipeline* pipe = track->efxPipe();
		if (!pipe)
			return;
	
		int size = pipe->size();
		QListWidgetItem *i = itemAt(dragPos);
		int idx0 = row(i);
		if (idx0 >= size)
			return;

		int distance = (dragPos - event->pos()).manhattanLength();
		if (distance > QApplication::startDragDistance())
		{
			//QListWidgetItem *i = itemAt(event->pos());
			//int idx = row(i);
			startDrag(idx0);
		}
	}
	QListWidget::mouseMoveEvent(event);
}

void EffectRack::initPlugin(Xml xml, int idx)/*{{{*/
{
    for (;;)
    {
        Xml::Token token = xml.parse();
        QString tag = xml.s1();
        switch (token)
        {
            case Xml::Error:
            case Xml::End:
                return;
            case Xml::TagStart:
                if (tag == "LadspaPlugin" || tag == "plugin")
                {
                    LadspaPlugin* ladplug = new LadspaPlugin();
                    if (ladplug->readConfiguration(xml, false))
                    {
                        printf("cannot instantiate plugin\n");
                        delete ladplug;
                    }
                    else
                    {
                        audio->msgAddPlugin(track, idx, ladplug);
                        song->update(SC_RACK);
                        return;
                    }
                }
                else if (tag == "Lv2Plugin")
                {
                    Lv2Plugin* lv2plug = new Lv2Plugin();
                    if (lv2plug->readConfiguration(xml, false))
                    {
                        printf("cannot instantiate plugin\n");
                        delete lv2plug;
                    }
                    else
                    {
                        audio->msgAddPlugin(track, idx, lv2plug);
                        song->update(SC_RACK);
                        return;
                    }
                }
                else if (tag == "VstPlugin")
                {
                    VstPlugin* vstplug = new VstPlugin();
                    if (vstplug->readConfiguration(xml, false))
                    {
                        printf("cannot instantiate plugin\n");
                        delete vstplug;
                    }
                    else
                    {
                        audio->msgAddPlugin(track, idx, vstplug);
                        song->update(SC_RACK);
                        return;
                    }
                }
                else if (tag == "oom")
                    break;
                else
                    xml.unknown("EffectRack");
                break;
            case Xml::Attribut:
                break;
            case Xml::TagEnd:
                if (tag == "oom")
                    return;
            default:
                break;
        }
    }
}/*}}}*/

