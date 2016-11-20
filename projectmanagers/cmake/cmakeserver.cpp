/* KDevelop CMake Support
 *
 * Copyright 2016 Aleix Pol <aleixpol@kde.org>
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

#include "cmakeserver.h"
#include "cmakeprojectdata.h"
#include "cmakeutils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

CMakeServer::CMakeServer(QObject* parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &CMakeServer::processOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, [this](){
        qWarning() << "cmake server:" << m_process.readAllStandardError();
    } );

    m_process.start(CMake::findExecutable(), {"-E", "server", "--debug"});
}

CMakeServer::~CMakeServer()
{
    m_process.kill();
    m_process.waitForFinished();
}

bool CMakeServer::isServerAvailable()
{
    if (m_process.state() == QProcess::Starting) {
        m_process.waitForStarted();
    }

    return m_process.state() == QProcess::Running;
}

void CMakeServer::command(const QJsonObject& object)
{
    const QByteArray data = "[== \"CMake Server\" ==[\n"
        + QJsonDocument(object).toJson() +
        "]== \"CMake Server\" ==]\n";
    auto len = m_process.write(data);
    Q_ASSERT(len>0);
    qDebug() << "xxxx" << object;
}

void CMakeServer::processOutput()
{
    static const QByteArray openTag  = "[== \"CMake Server\" ==[";
    static const QByteArray closeTag = "]== \"CMake Server\" ==]";

    m_buffer += m_process.readAllStandardOutput();
    qDebug() << "fuuuuuu" << m_buffer;
    for(; m_buffer.size() > openTag.size(); ) {

        Q_ASSERT(m_buffer.startsWith(openTag));
        int idx = m_buffer.indexOf(closeTag);
        if (idx>=0) {
            emitResponse(m_buffer.mid(openTag.size(), idx - openTag.size()));
            m_buffer = m_buffer.mid(idx + closeTag.size());
        }
        m_buffer += m_process.readAll();
    }
}

void CMakeServer::emitResponse(const QByteArray& data)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(data, &error);
    Q_ASSERT(doc.isObject());
    Q_ASSERT(!error.error);
    Q_EMIT response(doc.object());
}

void CMakeServer::hello()
{
    command({
        { "supportedProtocolVersions", QJsonArray { QJsonObject { {"major", 1}, {"minor", 0}}}},
        { "type", "hello" }
    });
}

void CMakeServer::handshake(const KDevelop::Path& source, const KDevelop::Path& build)
{
    command({
        {"cookie", ""},
        {"type", "handshake"},
        {"protocolVersion", {{"major", 0}} },
        {"sourceDirectory", source.toLocalFile()},
        {"buildDirectory", build.toLocalFile()},
        {"generator", "Ninja"} //TODO: make it possible to keep whatever they have ATM
    });
}

