/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo  <adymo@kdevelop.org>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "tooldocument.h"

#include <QWidget>

namespace Sublime {

// class ToolDocumentPrivate

class ToolDocumentPrivate
{
public:
    ~ToolDocumentPrivate()
    {
        delete factory;
    }
    ToolFactory *factory;
};



// class ToolDocument

ToolDocument::ToolDocument(const QString &title, Controller *controller, ToolFactory *factory)
    :Document(title, controller)
    , d_ptr(new ToolDocumentPrivate())
{
    Q_D(ToolDocument);

    d->factory = factory;
}

ToolDocument::~ToolDocument() = default;

ToolFactory *ToolDocument::factory() const
{
    Q_D(const ToolDocument);

    return d->factory;
}

QWidget *ToolDocument::createViewWidget(QWidget *parent)
{
    return factory()->create(this, parent);
}

QString ToolDocument::documentType() const
{
    return QStringLiteral("Tool");
}

QString ToolDocument::documentSpecifier() const
{
    return factory()->id();
}

}

