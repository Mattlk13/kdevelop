/*
    SPDX-FileCopyrightText: 2016 Milian Wolff <mail@milianw.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "test_stringhelpers.h"

#include <QTest>
#include <QStandardPaths>

#include <language/duchain/stringhelpers.h>

QTEST_MAIN(TestStringHelpers)

using namespace KDevelop;

void TestStringHelpers::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TestStringHelpers::testFormatComment_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("output");

    QTest::newRow("cpp-style") << QByteArrayLiteral(
        "// foo\n"
        "// bar"
    ) << QByteArrayLiteral("foo\n bar");

    QTest::newRow("doxy-cpp-style") << QByteArrayLiteral(
        "/// foo\n"
        "/// bar"
    ) << QByteArrayLiteral("foo\n bar");

    QTest::newRow("doxy-cpp-excl-style") << QByteArrayLiteral(
        "//! foo\n"
        "//! bar"
    ) << QByteArrayLiteral("foo\n bar");

    QTest::newRow("doxy-cpp-singleline-style") << QByteArrayLiteral("///< foo") << QByteArrayLiteral("foo");
    QTest::newRow("doxy-cpp-excl-singleline-style") << QByteArrayLiteral("//!< foo") << QByteArrayLiteral("foo");

    QTest::newRow("c-style") << QByteArrayLiteral(
        "/*\n"
        " foo\n"
        " bar\n*/"
    ) << QByteArrayLiteral("foo\nbar");

    QTest::newRow("doxy-c-style") << QByteArrayLiteral(
        "/**\n"
        " * foo\n"
        " * bar\n */"
    ) << QByteArrayLiteral("foo\n bar");

    QTest::newRow("doxy-c-style2") << QByteArrayLiteral(
        "/**\n"
        " * foo\n"
        " * bar\n **/"
    ) << QByteArrayLiteral("foo\n bar");

    QTest::newRow("real multiline") << QByteArrayLiteral(
                          "/**\n"
                          " * This is a real comment of some imaginary code.\n"
                          " *\n"
                          " * @param foo bar\n"
                          " * @return meh\n"
                          " */\n"
                      )
                    << QByteArrayLiteral("This is a real comment of some imaginary code.\n\n @param foo bar\n @return meh");
    QTest::newRow("doxy-qt-style-after-member") << QByteArrayLiteral(
        "/*!< line1\n"
        "line2 */"
    ) << QByteArrayLiteral("line1\nline2");

    QTest::newRow("doxy-c-style-after-member") << QByteArrayLiteral(
        "/**< line1\n"
        "line2 */"
    ) << QByteArrayLiteral("line1\nline2");

    QTest::newRow("doxy-cpp-style-after-member") << QByteArrayLiteral(
        "//!< line1\n"
        "//!< line2"
    ) << QByteArrayLiteral("line1\n line2");

    QTest::newRow("doxy-cpp-style-after-member2") << QByteArrayLiteral(
        "/// line1\n"
        "/// < line2"
    ) << QByteArrayLiteral("line1\n < line2");

    QTest::newRow("doxy-qt-style-before-member") << QByteArrayLiteral(
        "/*! line1\n"
        "line2 */"
    ) << QByteArrayLiteral("line1\nline2");

    QTest::newRow("doxy-qt-style-before-member2") << QByteArrayLiteral(
        "/*! line1\n"
        " * *line2* */"
    ) << QByteArrayLiteral("line1\n *line2*");

    QTest::newRow("doxy-cpp-style-before-member") << QByteArrayLiteral(
        "//! line1\n"
        "//! line2"
    ) << QByteArrayLiteral("line1\n line2");
}

void TestStringHelpers::testFormatComment()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, output);

    QCOMPARE(formatComment(input), output);
}

void TestStringHelpers::benchFormatComment()
{
    QBENCHMARK {
        formatComment(QByteArrayLiteral(
                          "/**\n"
                          " * This is a real comment of some imaginary code.\n"
                          " *\n"
                          " * @param foo bar\n"
                          " * @return meh\n"
                          " */\n"
                      ));
    }
}

void TestStringHelpers::testParamIterator_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QStringList>("params");

    auto addTest = [](const QString& source, const QStringList& params) {
        QTest::addRow("%s", qPrintable(source)) << source << params;
    };

    addTest("Empty", {});
    addTest("Foo<T1, T2>", {"T1", "T2"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator<(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator<(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator>(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator>(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator>=(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator>=(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator<=(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator<=(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator>>(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator>>(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator<<(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator<<(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator<=>(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator<=>(std::declval<_Up>()))>"});
    addTest("__not_overloaded2<_Tp, foo<bar>, __void_t<decltype(std::declval<_Tp>().operator->(std::declval<_Up>()))>>",
            {"_Tp", "foo<bar>", "__void_t<decltype(std::declval<_Tp>().operator->(std::declval<_Up>()))>"});
    addTest("A<\">\\\">\">", {"\">\\\">\""});
    addTest("A<'>'>", {"'>'"});
}

void TestStringHelpers::testParamIterator()
{
    QFETCH(QString, source);
    QFETCH(QStringList, params);

    const auto parens = QStringLiteral("<>:");

    auto it = KDevelop::ParamIterator(parens, source);

    while (!params.isEmpty()) {
        QVERIFY(it);
        QCOMPARE(*it, params.takeFirst());
        ++it;
    }

    QVERIFY(!it);
}
