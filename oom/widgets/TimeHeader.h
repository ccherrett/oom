#ifndef __OOM_TIMEHEADER_H__
#define __OOM_TIMEHEADER_H__

#include <QFrame>

class QCheckBox;
class QLabel;
class QVBoxLayout;
class PosLabel;

//---------------------------------------------------------
//   TimeHeader
//---------------------------------------------------------

class TimeHeader : public QFrame
{
    Q_OBJECT

    bool tickmode;
	QVBoxLayout* m_layout;

    bool setString(unsigned);

    PosLabel* cursorPos;
    QLabel *timeLabel, *posLabel, *frameLabel;

public slots:
    void setPos(int, unsigned, bool);
	void setTime(unsigned);

signals:
    void removeAllSolo();

public:
    TimeHeader(QWidget* parent);
};

#endif
