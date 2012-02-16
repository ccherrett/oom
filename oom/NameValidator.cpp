#include "NameValidator.h"
#include "track.h"
#include "song.h"

#include <QRegExp>

NameValidator::NameValidator(QObject *parent)
: QValidator(parent)
{
}

QValidator::State NameValidator::validate(QString& input, int& pos) const
{
	Q_UNUSED(pos);
	if(input == Track::getValidName(input) && QRegExp("[a-zA-Z0-9_ ]{0,255}").exactMatch(input))
	{
		return QValidator::Acceptable;
	}
	else
	{
		return QValidator::Invalid;
	}
}

void NameValidator::fixup(QString& input) const
{
	input = Track::getValidName(input);
}
