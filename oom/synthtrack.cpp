//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#include "synthtrack.h"
#include "xml.h"

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void SynthTrack::write(int level, Xml& xml) const
{
    xml.tag(level++, "SynthTrack");
    //AudioTrack::writeProperties(level, xml);
    xml.etag(level, "SynthTrack");
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void SynthTrack::read(Xml& xml)
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
            //if (AudioTrack::readProperties(xml, tag))
            xml.unknown("SynthTrack");
            break;
        case Xml::Attribut:
            break;
        case Xml::TagEnd:
            if (tag == "SynthTrack")
            {
                //mapRackPluginsToControllers();
                return;
            }
        default:
            break;
        }
    }
}

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QString SynthTrack::open()
{
    return QString("OK");
}

//---------------------------------------------------------
//   close
//---------------------------------------------------------

void SynthTrack::close()
{
    _readEnable = false;
    _writeEnable = false;
}
