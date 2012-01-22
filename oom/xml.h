//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: xml.h,v 1.8.2.3 2009/11/09 20:28:28 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __XML_H__
#define __XML_H__

#include <stdio.h>

#include <QString>

class QColor;
class QRect;
class QWidget;

//---------------------------------------------------------
//   Xml
//    very simple XML-like parser
//---------------------------------------------------------

class Xml
{
public:
    enum Token
    {
        Error, TagStart, TagEnd, Flag,
        Proc, Text, Attribut, End
    };

    Xml(FILE*);
    Xml(const char*);

    static QString xmlString(const char*);
    static QString xmlString(const QString&);

    int majorVersion() const
    {
        return _majorVersion;
    }

    int minorVersion() const
    {
        return _minorVersion;
    }

    void setVersion(int maj, int min)
    {
        _minorVersion = min;
        _majorVersion = maj;
    }

    // current line
    int line() const
    {
        return _line;
    }

    // current col
    int col() const
    {
        return _col;
    }

    const QString& s1()
    {
        return _s1;
    }

    const QString& s2()
    {
        return _s2;
    }

    Token parse();
    QString parse(const QString&);
    QString parse1();
    int parseInt();
    unsigned int parseUInt();
    float parseFloat();
    double parseDouble();
	qint64 parseLongLong();
    void unknown(const char*);

    void header();
    void put(const char* format, ...);
    void put(int level, const char* format, ...);
    void nput(int level, const char* format, ...);
    void nput(const char* format, ...);
    void tag(int level, const char* format, ...);
    void etag(int level, const char* format, ...);
    void intTag(int level, const char* const name, int val);
    void uintTag(int level, const char* const name, unsigned int val);
    void doubleTag(int level, const char* const name, double val);
    void qint64Tag(int level, const char* const name, qint64 val);
    void floatTag(int level, const char* const name, float val);
    void strTag(int level, const char* const name, const char* val);
    void strTag(int level, const char* const name, const QString& s);
    void colorTag(int level, const char* name, const QColor& color);
    void geometryTag(int level, const char* name, const QWidget* g);
    void qrectTag(int level, const char* name, const QRect& r);

    void skip(const QString& tag);
    void dump(QString &dump);

private:
    void next();
    void nextc();
    void token(int);
    void stoken();
    QString strip(const QString& s);
    void putLevel(int n);

    FILE* f;
    int _line;
    int _col;
    QString _s1, _s2, _tag;
    int level;
    bool inTag;
    bool inComment;
    int _minorVersion;
    int _majorVersion;

    int c; // current char
    char lbuffer[512];
    const char* bufptr;
};

extern QRect readGeometry(Xml&, const QString&);

#endif

