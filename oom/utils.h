//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: utils.h,v 1.1.1.1.2.3 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __UTILS_H__
#define __UTILS_H__

#include <Qt>
class QFrame;
class QString;
class QWidget;
class CItem;
class QDateTime;


extern QString bitmap2String(int bm);
extern int string2bitmap(const QString& str);
extern QString u32bitmap2String(unsigned int bm);
extern unsigned int string2u32bitmap(const QString& str);
extern bool autoAdjustFontSize(QFrame* w, const QString& s, bool ignoreWidth = false, bool ignoreHeight = false, int max = 10, int min = 4);

extern int num2cols(int min, int max);
extern QFrame* hLine(QWidget* parent);
extern QFrame* vLine(QWidget* parent);
extern void dump(const unsigned char* p, int n);
extern double curTime();

extern double dbToVal(double inDb);
extern double valToDb(double inV);
extern int dbToMidi(double val);
extern double midiToDb(int val);
extern double trackVolToDb(double val);
extern double dbToTrackVol(double val);
extern int trackPanToMidi(double val);
extern double midiToTrackPan(int val);
extern QString string2hex(const unsigned char* data, int len);
extern char* hex2string(const char* src, int& len, int& status);
extern qint64 create_id();
extern QDateTime extract_date_time(qint64 id);
extern QString midiControlToString(int ctrl);
extern int midiControlSortIndex(int ctrl);
extern int calcNRPN7(int, int);
extern QString sanitize(const QString);
extern QString trackTypeString(int);

extern  int getFreeMidiPort();
#endif

