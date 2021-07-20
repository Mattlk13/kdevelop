/***************************************************************************
 *   Copyright 2009 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#ifndef KDEVPLATFORM_PLUGIN_DOCUMENTSWITCHERPLUGIN_H
#define KDEVPLATFORM_PLUGIN_DOCUMENTSWITCHERPLUGIN_H

#include <interfaces/iplugin.h>
#include <QVariant>

namespace Sublime
{
    class MainWindow;
}

namespace KDevelop
{
    class IDocument;
}

class DocumentSwitcherTreeView;

class QStandardItemModel;
class QModelIndex;
class QAction;

class DocumentSwitcherPlugin: public KDevelop::IPlugin {
    Q_OBJECT
public:
    explicit DocumentSwitcherPlugin( QObject *parent, const QVariantList &args = QVariantList() );
    ~DocumentSwitcherPlugin() override;

    void unload() override;
public Q_SLOTS:
    void itemActivated( const QModelIndex& );
    void switchToClicked(const QModelIndex& );
    void walkForward();
    void walkBackward();
    void documentOpened(KDevelop::IDocument *document);
    void documentActivated(KDevelop::IDocument *document);
    void documentClosed(KDevelop::IDocument *document);
protected:
    bool eventFilter( QObject*, QEvent* ) override;
private:
    void setViewGeometry(Sublime::MainWindow* window);
    void enableActions();
    void fillModel();
    void walk(const int from, const int to);

    // List of opened document sorted activation.
    QList<KDevelop::IDocument *> documentLists;
    DocumentSwitcherTreeView* view;
    QStandardItemModel* model;
    QAction* forwardAction;
    QAction* backwardAction;
};

#endif

