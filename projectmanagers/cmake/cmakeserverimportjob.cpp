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
#include "cmakeserver.h"
#include <interfaces/iproject.h>
#include <QJsonArray>
#include <QRegularExpression>

template <typename T, typename Q, typename W>
static T kTransform(const Q& list, W func)
{
    T ret;
    ret.reserve(list.size());
    for (auto it = list.constBegin(), itEnd = list.constEnd(); it!=itEnd; ++it)
        ret += func(*it);
    return ret;
}

///copied from the makefile resolver
static QRegularExpression defineRegularExpression()
{
  static const QRegularExpression pattern(
    "-D([^\\s=]+)(?:=(?:\"(.*?)(?<!\\\\)\"|([^\\s]*)))?"
  );
  Q_ASSERT(pattern.isValid());
  return pattern;
}

static QString unescape(const QStringRef& input)
{
  QString output;
  output.reserve(input.length());
  bool isEscaped = false;
  for (auto it = input.data(), end = it + input.length(); it != end; ++it) {
    QChar c = *it;
    if (!isEscaped && c == '\\') {
      isEscaped = true;
    } else {
      output.append(c);
      isEscaped = false;
    }
  }
  return output;
}

static QHash<QString, QString> processDefines(const QString &compileFlags, const QJsonArray &defines)
{
    QHash<QString, QString> ret;
    const auto& defineRx = defineRegularExpression();
    auto it = defineRx.globalMatch(compileFlags);
    while (it.hasNext()) {
      const auto match = it.next();
      QString value;
      if (match.lastCapturedIndex() > 1) {
        value = unescape(match.capturedRef(match.lastCapturedIndex()));
      }
      ret[match.captured(1)] = value;
    }

    for(const QJsonValue& defineValue: defines) {
        const QString define = defineValue.toString();
        const int eqIdx = define.indexOf(QLatin1Char('='));
        if (eqIdx<0)
            ret[define] = QString();
        else
            ret[define.left(eqIdx)] = define.mid(eqIdx+1);
    }
    return ret;
}

static void processFileData(const QJsonObject &response, CMakeProjectData &data)
{
    const auto configs = response.value("configurations").toArray();
    qDebug() << "fuuuuuu" << response;
    for (const auto &config: configs) {
        const auto projects = config.toObject().value("projects").toArray();
        for (const auto &project: projects) {
            const auto targets = project.toObject().value("targets").toArray();
            for (const auto &targetObject: targets) {
                const auto target = targetObject.toObject();
                const KDevelop::Path targetDir(target.value("sourceDirectory").toString());

                data.targets[targetDir] += target.value("name").toString();

                const auto fileGroups = target.value("fileGroups").toArray();
                for (const auto &fileGroupValue: fileGroups) {
                    const auto fileGroup = fileGroupValue.toObject();
                    CMakeFile file;
                    file.includes = kTransform<KDevelop::Path::List>(fileGroup.value("includePath").toArray(), [](const QJsonValue& val) { return KDevelop::Path(val.toObject().value("path").toString()); });
                    file.defines = processDefines(fileGroup.value("compileFlags").toString(), fileGroup.value("defines").toArray());

                    const auto sourcesArray = fileGroup.value("sources").toArray();
                    const KDevelop::Path::List sources = kTransform<KDevelop::Path::List>(sourcesArray, [targetDir](const QJsonValue& val) { return KDevelop::Path(targetDir, val.toString()); });
                    for (const auto& source: sources) {
                        data.compilationData.files[source] = file;
                    }
                    qDebug() << "registering..." << sources << file;
                }
            }
        }
    }
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
    connect(m_server, &CMakeServer::response, this, [this](const QJsonObject &response) {
        if (response.value("type") == QLatin1String("reply")) {
            const auto inReplyTo = response.value("inReplyTo");
            qDebug() << "replying..." << inReplyTo;
            if (inReplyTo == "handshake")
                m_server->configure({});
            else if (inReplyTo == QLatin1String("configure"))
                m_server->compute();
            else if (inReplyTo == QLatin1String("compute"))
                m_server->codemodel();
            else if(inReplyTo == QLatin1String("codemodel")) {
                processFileData(response, m_data);
                emitResult();
            } else
                qWarning() << "unhandled reply" << response;
        } else if(response.value("type") == QLatin1String("error")) {
            setError(2);
            setErrorText(response.value("errorMessage").toString());
            qWarning() << "error!!" << response;
            emitResult();
        } else {
            qWarning() << "unhandled message" << response;
        }
    });

    m_server->handshake(m_project->path(), CMake::currentBuildDir(m_project));
}
