/*
Copyright (C) 2006-2011 Remon Sijrier

This file is part of Traverso

Traverso is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.

*/

#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QHash>
#include <QVariant>

class TConfig : public QObject
{
	Q_OBJECT
public:
	QVariant get_property(const QString& type, const QString& property, QVariant defaultValue);
	void set_property(const QString& type, const QString& property, QVariant newValue);
	
	void check_and_load_configuration();
	void reset_settings( );
	
	void save();

signals:
	void configChanged();

private:
        TConfig() {}
        ~TConfig();

        TConfig(const TConfig&);

	// allow this function to create one instance
        friend TConfig& tconfig();
	
	void load_configuration();
	
	QHash<QString, QVariant>	m_configs;

};


// use this function to access the settings
TConfig& tconfig();


#endif
