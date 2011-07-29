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

#include "TConfig.h"
#include "config.h"

#include <QSettings>
#include <QString>
#include <QDir>

static const char* CONFIG_FILE_VERSION = "1";


TConfig& tconfig()
{
        static TConfig conf;
	return conf;
}

TConfig::~ TConfig( )
{
}

void TConfig::load_configuration()
{
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, PACKAGE_NAME, "oom-"VERSION);
	
	QStringList keys = settings.allKeys();
	
	foreach(const QString &key, keys) {
		m_configs.insert(key, settings.value(key));
	}
}

void TConfig::reset_settings( )
{
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, PACKAGE_NAME, "oom-"VERSION);

	settings.clear();

        settings.setValue("ProgramVersion", 2);
	settings.setValue("ConfigFileVersion", CONFIG_FILE_VERSION);
	
	m_configs.clear();
	
	load_configuration();
}

void TConfig::check_and_load_configuration( )
{
        QSettings::setPath (QSettings::IniFormat, QSettings::UserScope, QDir::homePath () + "/.config");

        load_configuration();

	// Detect if the config file versions match, if not, there has been most likely 
	// a change, overwrite with the newest version...
	if (m_configs.value("ConfigFileVersion").toString() != CONFIG_FILE_VERSION) {
		reset_settings();
	}
}

void TConfig::save( )
{
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, PACKAGE_NAME, "oom-"VERSION);

	QHash<QString, QVariant>::const_iterator i = m_configs.constBegin();
	
	while (i != m_configs.constEnd()) {
		settings.setValue(i.key(), i.value());
		++i;
	}
	
	emit configChanged();
}

QVariant TConfig::get_property( const QString & type, const QString & property, QVariant defaultValue )
{
	QVariant var = defaultValue;
	QString key = type + ("/") + property;
	
	if (m_configs.contains(key)) {
		var = m_configs.value(key);
	} else {
		m_configs.insert(key, defaultValue);
	}
	
	return var;
}

void TConfig::set_property( const QString & type, const QString & property, QVariant newValue )
{
	m_configs.insert(type + "/" + property, newValue);
}
