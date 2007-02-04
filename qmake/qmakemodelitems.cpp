/* KDevelop QMake Support
 *
 * Copyright 2006 Andreas Pakulat <apaku@gmx.de>
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

#include "qmakemodelitems.h"

#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QList>

#include "qmakeprojectscope.h"

QMakeFolderItem::QMakeFolderItem( QMakeProjectScope* scope, const KUrl& url, QStandardItem* parent )
: Koncrete::ProjectFolderItem( url, parent ), m_projectScope( scope )
{
}

QMakeProjectScope* QMakeFolderItem::projectScope() const
{
    return m_projectScope;
}

QMakeFolderItem::~QMakeFolderItem()
{
}

struct QMakeTargetItemPrivate
{
    KUrl::List m_includes;
    QHash<QString, QString> m_env;
    QList<QPair<QString,QString> > m_defs;
};

QMakeTargetItem::QMakeTargetItem( const QString& s, QStandardItem* parent )
    : Koncrete::ProjectTargetItem( s, parent ), d(new QMakeTargetItemPrivate)
{
}

QMakeTargetItem::~QMakeTargetItem()
{
    delete d;
}

const KUrl::List& QMakeTargetItem::includeDirectories() const
{
    return d->m_includes;
}

const QHash<QString, QString>& QMakeTargetItem::environment() const
{
    return d->m_env;
}

const QList<QPair<QString, QString> >& QMakeTargetItem::defines() const
{
    return d->m_defs;
}

// kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on; auto-insert-doxygen on
