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
#include <KRandom>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

CMakeServer::CMakeServer(QObject* parent)
    : QObject(parent)
    , m_localSocket(new QLocalSocket(this))
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/kdevelop-cmake-" + QString::number(KRandom::random());

    m_process.setProcessChannelMode(QProcess::ForwardedChannels);
    m_process.start(CMake::findExecutable(), {"-E", "server", "--experimental", "--pipe=" + path});

    connect(&m_process, &QProcess::started, this, [this, path]() {
        m_localSocket->connectToServer(path, QIODevice::ReadWrite);
    });

    connect(m_localSocket, &QIODevice::readyRead, this, &CMakeServer::processOutput);
    connect(m_localSocket, &QLocalSocket::connected, this, &CMakeServer::connected);
}

CMakeServer::~CMakeServer()
{
    m_process.kill();
    m_process.waitForFinished();
}

bool CMakeServer::isServerAvailable()
{
    return m_localSocket->isOpen();
}

static const QByteArray openTag  = "\n[== \"CMake Server\" ==[\n";
static const QByteArray closeTag = "\n]== \"CMake Server\" ==]\n";

void CMakeServer::command(const QJsonObject& object)
{
    Q_ASSERT(m_localSocket->isOpen());

    const QByteArray data = openTag + QJsonDocument(object).toJson(QJsonDocument::Compact) + closeTag;
    auto len = m_localSocket->write(data);
//     qDebug() << "writing..." << data;
    Q_ASSERT(len>0);
}

void CMakeServer::processOutput()
{
    Q_ASSERT(m_localSocket);

    m_buffer += m_localSocket->readAll();
    for(; m_buffer.size() > openTag.size(); ) {

        Q_ASSERT(m_buffer.startsWith(openTag));
        const int idx = m_buffer.indexOf(closeTag, openTag.size());
        if (idx>=0) {
            emitResponse(m_buffer.mid(openTag.size(), idx - openTag.size()));
            m_buffer = m_buffer.mid(idx + closeTag.size());
        }
    }
}

void CMakeServer::emitResponse(const QByteArray& data)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error) {
        qWarning() << "error processing" << error.errorString() << data;
    }
    Q_ASSERT(!error.error);
    Q_ASSERT(doc.isObject());
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
    Q_ASSERT(!source.isEmpty());
    Q_ASSERT(!build.isEmpty());

    command({
        {"cookie", ""},
        {"type", "handshake"},
        {"protocolVersion", QJsonObject{{"major", 1}} },
        {"sourceDirectory", source.toLocalFile()},
        {"buildDirectory", build.toLocalFile()},
        {"generator", "Ninja"} //TODO: make it possible to keep whatever they have ATM
    });
}

void CMakeServer::configure(const QStringList& args)
{
    command({
        {"type", "configure"},
        {"cacheArguments", QJsonArray::fromStringList(args)}
    });
}

void CMakeServer::compute()
{
    command({ {"type", "compute"} });
}

void CMakeServer::codemodel()
{
    command({ {"type", "codemodel"} });
}
