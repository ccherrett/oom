//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================


#include "DoubleSlider.h"
#include <QtGui>

SliderSplitter::SliderSplitter(Qt::Orientation orient, QWidget* parent)
: QSplitter(orient, parent)
{
}

void SliderSplitter::moveSlider(int pos, int index)
{
	this->moveSplitter(pos, index);
}

SliderMiddle::SliderMiddle(QWidget* parent)
: QFrame(parent)
{
	m_moving = false;
	m_startMoving = false;
	m_lastX = 0;
	setCursor(Qt::PointingHandCursor);
}

void SliderMiddle::mousePressEvent(QMouseEvent* ev)
{
	if(ev->button() == Qt::LeftButton)
	{
		m_startPos = QCursor::pos();
		m_startMoving = true;
	}
}

void SliderMiddle::mouseMoveEvent(QMouseEvent* ev)
{
	Q_UNUSED(ev);
	if(m_startMoving)
	{
		if ((QCursor::pos() - m_startPos).manhattanLength() < QApplication::startDragDistance())
		{
			m_moving = true;
		}
	}

	if(m_moving)
	{
		int x = QCursor::pos().x();
		int startX = m_startPos.x();
		m_startPos = QCursor::pos();

		m_lastX = x - startX;
		if(m_lastX == 0)
			return;
		if(startX < x)
		{//Moving right
			emit moveRight(x-startX);
		}
		else if(x < startX)
		{
			emit moveLeft(startX-x);
		}
	}
}

void SliderMiddle::mouseReleaseEvent(QMouseEvent* ev)
{
	Q_UNUSED(ev);
	m_moving = false;
	m_startMoving = false;
	m_lastX = 0;
}


DoubleSlider::DoubleSlider(bool showLabels, QWidget *parent)
	: QFrame(parent)
{
	// default values
	m_handleWidth = 8;
	_valueMin = 30.0;
	_valueMax = 180.0;
	_currentMin = 80.0;
	_currentMax = 120.0;
	_minLabel = 0;
	_maxLabel = 0;

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0,0,0,0);

	_box = new QFrame();
	_box->setObjectName("doubleSliderContainer");
	QHBoxLayout *boxLayout = new QHBoxLayout(_box);
	boxLayout->setSpacing(0);
	boxLayout->setContentsMargins(0,0,0,0);
	
	_before = new QFrame(this);
	_before->setObjectName("doubleSliderLeft");
	
	_selection = new SliderMiddle(this);
	_selection->setObjectName("doubleSliderMiddle");
	_selection->setMinimumWidth(1);
	
	_after = new QFrame(this);
	_after->setObjectName("doubleSliderRight");
	
	_splitter = new SliderSplitter(Qt::Horizontal, this);
	_splitter->addWidget(_before);
	_splitter->addWidget(_selection);
	_splitter->addWidget(_after);
	_splitter->setHandleWidth(m_handleWidth);
	_splitter->setCollapsible(0, true);
	_splitter->setCollapsible(1, false);
	_splitter->setCollapsible(2, true);
	_splitter->handle(1)->setCursor(Qt::PointingHandCursor);
	_splitter->handle(2)->setCursor(Qt::PointingHandCursor);
	
	boxLayout->addWidget(_splitter);
	
	if(showLabels)
	{
		_minLabel = new QLabel();
		_maxLabel = new QLabel();
		_minLabel->setFixedWidth(30);
		_minLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		_maxLabel->setFixedWidth(30);
		_maxLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

		layout->addWidget(_minLabel);
		layout->addSpacing(2);
	}
	layout->addWidget(_box, 10);
	
	if(showLabels)
	{
		layout->addSpacing(2);
		layout->addWidget(_maxLabel);

		/*QObject::connect(
			this, SIGNAL(minChanged(double)),
			this, SLOT(updateLabels())
		);
		QObject::connect(
			this, SIGNAL(maxChanged(double)),
			this, SLOT(updateLabels())
		);*/
	}
	
	connect(
		_selection, SIGNAL(moveLeft(int)),
		this, SLOT(moveLeft(int))
	);
	connect(
		_selection, SIGNAL(moveRight(int)),
		this, SLOT(moveRight(int))
	);
	QObject::connect(
		_splitter, SIGNAL(splitterMoved(int, int)),
		this, SLOT(updateValues(int, int))
	);
	
	this->updateMinPos();
	this->updateMaxPos();

	this->updateLabels();
}

DoubleSlider::~DoubleSlider()
{
}

QSize DoubleSlider::sizeHint() const
{
    return QSize(180, 12);
}

double DoubleSlider::currentMin()
{
	return _currentMin;
}

double DoubleSlider::currentMax()
{
	return _currentMax;
}

double DoubleSlider::valueMin()
{
	return _valueMax;
}

double DoubleSlider::valueMax()
{
	return _valueMax;
}

void DoubleSlider::setCurrentMin(double currentMin)
{
	if (currentMin >= _valueMin)
		_currentMin = currentMin;
		
	if (_currentMin > _currentMax)
	{
		this->updateCurrentMax(currentMin+1);
		//emit maxChanged(_currentMax);
	}
		
	this->updateMinPos();
	if(_minLabel)
		_minLabel->setText(QString::number(qRound(currentMin)));
	//updateLabels();
}

void DoubleSlider::setCurrentMax(double currentMax)
{
	if (currentMax <= _valueMax)
		_currentMax = currentMax;
		
	if (_currentMax < _currentMin)
	{
		this->updateCurrentMin(currentMax-1);
		//emit minChanged(_currentMin);
	}
		
	this->updateMaxPos();
	if(_maxLabel)
		_maxLabel->setText(QString::number(qRound(currentMax)));
	//updateLabels();
}
	
void DoubleSlider::updateCurrentMin(double currentMin)
{
	if (currentMin >= _valueMin)
		_currentMin = currentMin;
		
	if (_currentMin > _currentMax)
	{
		this->updateCurrentMax(currentMin+1);
		emit maxChanged(_currentMax);
	}
	emit minChanged(_currentMin);
	this->updateMinPos();
	if(_minLabel)
		_minLabel->setText(QString::number(qRound(currentMin)));
	//updateLabels();
}

void DoubleSlider::updateCurrentMax(double currentMax)
{
	if (currentMax <= _valueMax)
		_currentMax = currentMax;
		
	if (_currentMax < _currentMin)
	{
		this->updateCurrentMin(currentMax-1);
		emit minChanged(_currentMin);
	}
	emit maxChanged(_currentMax);
		
	this->updateMaxPos();
	if(_maxLabel)
		_maxLabel->setText(QString::number(qRound(currentMax)));
	//updateLabels();
}

void DoubleSlider::setRange(double min, double max)
{
	if (min < _valueMax)
		_valueMin = min;

	if (_currentMin < _valueMin)
		updateCurrentMin(_valueMin);
	else
		updateCurrentMin(_currentMin);
	
	if (max > _valueMin)
		_valueMax = max;

	if (_currentMax > _valueMax)
		updateCurrentMax(_valueMax);
	else
		updateCurrentMax(_currentMax);

	//updateLabels();
}

void DoubleSlider::setValueMin(double valueMin)
{
	if (valueMin < _valueMax)
		_valueMin = valueMin;

	if (_currentMin < _valueMin)
		_currentMin = _valueMin;
	
	this->updateMinPos();
	this->updateMaxPos();
	//updateLabels();
}

void DoubleSlider::setValueMax(double valueMax)
{
	if (valueMax > _valueMin)
		_valueMax = valueMax;

	if (_currentMax > _valueMax)
		_currentMax = _valueMax;

	this->updateMinPos();
	this->updateMaxPos();
	//updateLabels();
}

void DoubleSlider::updateMinPos()
{
	QSize size = _before->size();
	size.setWidth(qRound(
		(_currentMin - _valueMin) *
		(double)(_before->width() + _selection->width() + _after->width()) /
		(_valueMax - _valueMin)));
	_before->resize(size);	
}

void DoubleSlider::updateMaxPos()
{
	QSize size = _after->size();
		size.setWidth(qRound(
		(_valueMax - _currentMax) *
		(double)(_before->width() + _selection->width() + _after->width()) /
		(_valueMax - _valueMin)));
	_after->resize(size);
}

void DoubleSlider::updateValues(int pos, int index)
{
	int total = _box->width()-(m_handleWidth*2);
	if (index == 1)
	{//Left handle moved
		double currentMin = (_valueMax - _valueMin) / (double)(total) * pos + _valueMin;
		updateCurrentMin(currentMin);
		//qDebug("DoubleSlider::updateValues: _currentMax: %.02f, currentMin: %.02f", _currentMax, currentMin);
		//emit minChanged(currentMin);
		if(_minLabel)
			_minLabel->setText(QString::number(qRound(currentMin)));
	}
	if (index == 2)
	{//Right handle moved
		double  currentMax = (_valueMax - _valueMin) / (double)(total) * (pos - m_handleWidth) + _valueMin;
		updateCurrentMax(currentMax);
		//qDebug("DoubleSlider::updateValues: currentMax: %.02f, _currentMin: %.02f", currentMax, _currentMin);
		//emit maxChanged(currentMax);
		if(_maxLabel)
			_maxLabel->setText(QString::number(qRound(currentMax)));
	}
}

void DoubleSlider::moveLeft(int val)
{
	if(!val)
		return;
	QSize bsize = _before->size();
	QSize asize = _after->size();
	int selwidth = _selection->size().width();
	int bwidth =  bsize.width() - val;
	if(bwidth < 0)
	{
		val = val - (val - bsize.width());
		bwidth =  bsize.width() - val;
	}
	int lpos = bwidth;
	int rpos = (bwidth+selwidth)+m_handleWidth;

	//qDebug("DoubleSlider::moveLeft: val: %d, bwidth: %d, rpos: %d, lpos: %d", val, bwidth, rpos, lpos);
	_splitter->moveSlider(lpos, 1);
	_splitter->moveSlider(rpos, 2);
	
}

void DoubleSlider::moveRight(int val)
{
	if(!val)
		return;
	QSize asize = _after->size();
	QSize bsize = _before->size();
	int boxWidth = _box->width();
	int total = boxWidth-(m_handleWidth*2);
	int selwidth = _selection->size().width();
	int bwidth =  bsize.width() + val;
	if((bwidth+selwidth) > total)
	{
		int rem = (bwidth+selwidth) - total;
		bwidth =  bwidth - rem;
	}
	int lpos = bwidth;
	int rpos = (bwidth+selwidth)+m_handleWidth;

	//qDebug("DoubleSlider::moveRight: val: %d, bwidth: %d, rpos: %d, lpos: %d", val, bwidth, rpos, lpos);
	_splitter->moveSlider(lpos, 1);
	_splitter->moveSlider(rpos, 2);
}

void DoubleSlider::updateLabels()
{
	//qDebug("DoubleSlider::updateLabels: _currentMax: %.02f, _currentMin: %.02f", _currentMax, _currentMin);
	if(_minLabel)
		_minLabel->setText(QString::number(qRound(_currentMin)));
	if(_maxLabel)
		_maxLabel->setText(QString::number(qRound(_currentMax)));
}
