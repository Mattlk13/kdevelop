/*
 * This file is part of KDevelop
 *
 * Copyright 2015 Sergey Kalinichev <kalinichev.so.0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
#include <QScopedPointer>

#include "clangsettingsmanager.h"

namespace Ui
{
class ConfigWidget;
}

namespace KDevelop
{
class IProject;
}

class PathsModel;

class ConfigWidget : public QWidget
{
    Q_OBJECT
public:
    ConfigWidget(QWidget* parent, KDevelop::IProject* project);
    ~ConfigWidget();

    void writeSettings();
    void defaults();

signals:
    void changed();

private slots:
    void itemActivated();
    void addPath();
    void deletePath();
    void textEdited();
    void languageStandardChanged(const QString& standard);

private:
    QScopedPointer<Ui::ConfigWidget> m_ui;
    KDevelop::IProject* m_project;
    PathsModel* m_model;
};

#endif // CONFIGWIDGET_H
