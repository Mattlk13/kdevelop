#include <klineedit.h>
#include <qlistview.h>

#include "kcomboview.h"

KComboView::KComboView( bool rw, QWidget* parent, const char* name )
    :QComboView(rw, parent, name)
{
    if (rw)
    {
        KLineEdit *ed = new KLineEdit(this, "combo edit");
        ed->setCompletionMode(KGlobalSettings::CompletionPopup);
        ed->setCompletionObject(&m_comp);
        setLineEdit(ed);
    }
}

void KComboView::addItem(QListViewItem *it)
{
    m_comp.addItem(it->text(0));
}

void KComboView::removeItem(QListViewItem *it)
{
    m_comp.removeItem(it->text(0));
    delete it;
}

void KComboView::renameItem(QListViewItem *it, const QString &newName)
{
    m_comp.removeItem(it->text(0));
    it->setText(0, newName);
    m_comp.addItem(newName);
}

void KComboView::clear( )
{
    m_comp.clear();
    QComboView::clear();
}
