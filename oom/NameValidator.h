#ifndef __OOM_NAME_VALIDATOR__
#define __OOM_NAME_VALIDATOR__

#include <QValidator>

class NameValidator : public QValidator
{
	Q_OBJECT
public:
	NameValidator(QObject* parent = 0);
	virtual QValidator::State validate(QString& input, int& pos) const;
	virtual void fixup(QString& input) const;
};

#endif
