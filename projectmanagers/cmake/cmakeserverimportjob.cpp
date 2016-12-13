/* KDevelop CMake Support
 *
 * Copyright 2014 Kevin Funk <kfunk@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "cmakeserverimportjob.h"
#include "cmakeutils.h"
#include <interfaces/iproject.h>
#include "cmakeserver.h"

static void processFileData(const QJsonObject &response, CMakeProjectData &data)
{
    qDebug() << "fuuuuuuuu" << response;
}

CMakeServerImportJob::CMakeServerImportJob(KDevelop::IProject* project, QObject* parent)
    : KJob(parent)
    , m_server(new CMakeServer(this))
    , m_project(project)
{
    connect(m_server, &CMakeServer::disconnected, this, [this]() {
        setError(1);
        emitResult();
    });
}

void CMakeServerImportJob::start()
{
    if (m_server->isServerAvailable())
        doStart();
    else
        connect(m_server, &CMakeServer::connected, this, &CMakeServerImportJob::doStart);
}

void CMakeServerImportJob::doStart()
{
    connect(m_server, &CMakeServer::response, this, [this](const QJsonObject &response){
        qDebug() << "xxxxxxxx" << response;
        if (response.value("inReplyTo") == QLatin1String("handshake") && response.value("type") == QLatin1String("reply"))
            m_server->command({{"type", "codemodel"}});
        else if(response.value("inReplyTo") == QLatin1String("codemodel")) {
            processFileData(response, m_data);
            emitResult();
        }
    });

    m_server->handshake(m_project->path(), CMake::currentBuildDir(m_project));
}
