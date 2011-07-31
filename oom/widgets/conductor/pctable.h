#ifndef _PCTABLE_
#define _PCTABLE_

#include <QTableView>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QPaintEvent>
#include <QList>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyleOptionViewItemV4>
#include <QTextDocument>
#include <QModelIndex>

class ProgramChangeTable : public QTableView
{
    Q_OBJECT
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dragMoveEvent(QDragMoveEvent*);
    //virtual void paintEvent(QPaintEvent*);
    QRect dropSite;

signals:
    void rowOrderChanged();

public:
    ProgramChangeTable(QWidget *parent = 0);
    void dropEvent(QDropEvent *evt);
    void mousePressEvent(QMouseEvent* evt);

public slots:
    QList<int> getSelectedRows();
};

class HTMLDelegate : public QStyledItemDelegate
{
    Q_OBJECT

protected:
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
};


#endif
