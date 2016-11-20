/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
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

#include <cmakeserver.h>
#include <QtTest>

using namespace KDevelop;

class CMakeServerTest : public QObject
{
Q_OBJECT
private slots:
    void testRun()
    {
        CMakeServer server(this);
        QJsonObject codeModel;
        QSignalSpy spy(&server, &CMakeServer::response);
        QVERIFY(spy.wait());

        connect(&server, &CMakeServer::response, this, [&codeModel, &server](const QJsonObject &response) {
            if (response.value("inReplyTo") == QLatin1String("handshake") && response.value("type") == QLatin1String("reply"))
                server.command({{"type", "codemodel"}});
            else if(response.value("inReplyTo") == QLatin1String("codemodel"))
                codeModel = response;
        });

        server.handshake({}, {});
        while(codeModel.isEmpty()) {
            spy.clear();
            QVERIFY(spy.wait());
        }
    }
};

QTEST_MAIN( CMakeServerTest )

#include "test_cmakeserver.moc"
