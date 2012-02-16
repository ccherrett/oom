#include "NameValidator.h"
#include "track.h"
#include "song.h"

#include <QRegExp>

NameValidator::NameValidator(QObject *parent)
: QValidator(parent)
{
}

QValidator::State NameValidator::validate(QString& name, int& pos) const
{
	Q_UNUSED(pos);
	if(name == Track::getValidName(name) && QRegExp("[a-zA-Z0-9_- ]{0,255}").exactMatch(name))
	{
		return QValidator::Acceptable;
	}
	else
	{
		fixup(name);
		if(QRegExp("[a-zA-Z0-9_- ]{0,255}").exactMatch(name))
			return QValidator::Intermediate;
		else
			return QValidator::Invalid;
	}
}

void NameValidator::fixup(QString& input) const
{
	input = Track::getValidName(input);
}
