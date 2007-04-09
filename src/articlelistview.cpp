/*
    This file is part of Akregator.

    Copyright (C) 2004 Stanislav Karchebny <Stanislav.Karchebny@kdemail.net>
                  2005 Frank Osterfeld <frank.osterfeld at kdemail.net>
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "actionmanager.h"
#include "akregatorconfig.h"
#include "articlelistview.h"
#include "article.h"
#include "articlefilter.h"
#include "dragobjects.h"
#include "feed.h"
#include "treenode.h"
#include "treenodevisitor.h"

#include <kcharsets.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <q3header.h>
#include <q3simplerichtext.h>

#include <QApplication>
#include <QDateTime>
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>

#include <ctime>

namespace Akregator {

class ArticleListView::ArticleListViewPrivate
{
    public:

    ArticleListViewPrivate(ArticleListView* parent) : m_parent(parent) { }

    void ensureCurrentItemVisible()
    {
        if (m_parent->currentItem())
        {
            m_parent->center( m_parent->contentsX(), m_parent->itemPos(m_parent->currentItem()), 0, 9.0 );
        }
    }

    ArticleListView* m_parent;

    /** maps article to article item */
    QHash<QString, ArticleItem*> articleMap;
    TreeNode* node;
    Akregator::Filters::ArticleMatcher textFilter;
    Akregator::Filters::ArticleMatcher statusFilter;
    enum ColumnMode { groupMode, feedMode };
    ColumnMode columnMode;
    int feedWidth;
    bool noneSelected;

    ColumnLayoutVisitor* columnLayoutVisitor;
};

class ArticleListView::ColumnLayoutVisitor : public TreeNodeVisitor
{
    public:
        ColumnLayoutVisitor(ArticleListView* view) : m_view(view) {}

        virtual bool visitTagNode(TagNode* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::feedMode)
            {
                m_view->setColumnWidth(1, m_view->d->feedWidth);
                m_view->d->columnMode = ArticleListViewPrivate::groupMode;
            }
            return true;
        }

        virtual bool visitFolder(Folder* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::feedMode)
            {
                m_view->setColumnWidth(1, m_view->d->feedWidth);
                m_view->d->columnMode = ArticleListViewPrivate::groupMode;
            }
            return true;
        }

        virtual bool visitFeed(Feed* /*node*/)
        {
            if (m_view->d->columnMode == ArticleListViewPrivate::groupMode)
            {
                m_view->d->feedWidth = m_view->columnWidth(1);
                m_view->hideColumn(1);
                m_view->d->columnMode = ArticleListViewPrivate::feedMode;
            }
            return true;
        }
    private:

        ArticleListView* m_view;

};

class ArticleListView::ArticleItem : public K3ListViewItem
    {
        friend class ArticleListView;

        public:
            ArticleItem( Q3ListView *parent, const Article& a);
            ~ArticleItem();

            Article& article();

            void paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align );
            virtual int compare(Q3ListViewItem *i, int col, bool ascending) const;

            void updateItem(const Article& article);

            virtual ArticleItem* itemAbove() { return static_cast<ArticleItem*>(K3ListViewItem::itemAbove()); }

            virtual ArticleItem* nextSibling() { return static_cast<ArticleItem*>(K3ListViewItem::nextSibling()); }

        private:
            Article m_article;
            time_t m_pubDate;
            static QPixmap m_keepFlag;
};

ArticleListView::ArticleItem::ArticleItem( Q3ListView *parent, const Article& a)
    : K3ListViewItem( parent, a.title(), a.feed()->title(), KGlobal::locale()->formatDateTime(a.pubDate(), KLocale::ShortDate, false) ), m_article(a), m_pubDate(a.pubDate().toTime_t())
{
    if (a.keep())
        setPixmap(0, m_keepFlag);
}

ArticleListView::ArticleItem::~ArticleItem()
{
}

Article& ArticleListView::ArticleItem::article()
{
    return m_article;
}

QPixmap ArticleListView::ArticleItem::m_keepFlag = QPixmap(KStandardDirs::locate("data", "akregator/pics/akregator_flag.png"));

// paint ze peons
void ArticleListView::ArticleItem::paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    if (article().status() == Article::Read)
        K3ListViewItem::paintCell( p, cg, column, width, align );
    else
    {
        // if article status is unread or new, we change the color: FIXME: make colors configurable
        QColorGroup cg2(cg);

        if (article().status() == Article::Unread)
            cg2.setColor(QColorGroup::Text, Settings::useCustomColors() ?
                    Settings::colorUnreadArticles() : Qt::blue);
        else // New
            cg2.setColor(QColorGroup::Text, Settings::useCustomColors() ?
                    Settings::colorNewArticles() : Qt::red);

        K3ListViewItem::paintCell( p, cg2, column, width, align );
    }

}

void ArticleListView::ArticleItem::updateItem(const Article& article)
{
    m_article = article;
    setPixmap(0, m_article.keep() ? m_keepFlag : QPixmap());
    setText(0, KCharsets::resolveEntities(m_article.title()));
    setText(1, m_article.feed()->title());
    setText(2, KGlobal::locale()->formatDateTime(m_article.pubDate(), KLocale::ShortDate, false));
}

int ArticleListView::ArticleItem::compare(Q3ListViewItem *i, int col, bool ascending) const {
    if (col == 2)
    {
        ArticleItem* item = static_cast<ArticleItem*>(i);
        if (m_pubDate == item->m_pubDate)
            return 0;
        return (m_pubDate > item->m_pubDate) ? 1 : -1;
    }
    return K3ListViewItem::compare(i, col, ascending);
}

/* ==================================================================================== */

ArticleListView::ArticleListView(QWidget *parent, const char *name)
    : K3ListView(parent)
{
    d = new ArticleListViewPrivate(this);
    setObjectName(name);
    d->noneSelected = true;
    d->node = 0;
    d->columnMode = ArticleListViewPrivate::feedMode;

    d->columnLayoutVisitor = new ColumnLayoutVisitor(this);
    setMinimumSize(250, 150);
    addColumn(i18n("Article"));
    addColumn(i18n("Feed"));
    addColumn(i18n("Date"));
    setSelectionMode(Q3ListView::Extended);
    setColumnWidthMode(2, Q3ListView::Maximum);
    setColumnWidthMode(1, Q3ListView::Manual);
    setColumnWidthMode(0, Q3ListView::Manual);
    setRootIsDecorated(false);
    setItemsRenameable(false);
    setItemsMovable(false);
    setAllColumnsShowFocus(true);
    setDragEnabled(true); // FIXME before we implement dragging between archived feeds??
    setAcceptDrops(false); // FIXME before we implement dragging between archived feeds??
    setFullWidth(false);

    setShowSortIndicator(true);
    setDragAutoScroll(true);
    setDropHighlighter(false);

    int c = Settings::sortColumn();
    setSorting((c >= 0 && c <= 2) ? c : 2, Settings::sortAscending());

    int w;
    w = Settings::titleWidth();
    if (w > 0) {
        setColumnWidth(0, w);
    }

    w = Settings::feedWidth();
    if (w > 0) {
        setColumnWidth(1, w);
    }

    w = Settings::dateWidth();
    if (w > 0) {
        setColumnWidth(2, w);
    }

    d->feedWidth = columnWidth(1);
    hideColumn(1);

    header()->setStretchEnabled(true, 0);

    this->setWhatsThis( i18n("<h2>Article list</h2>"
        "Here you can browse articles from the currently selected feed. "
        "You can also manage articles, as marking them as persistent (\"Keep Article\") or delete them, using the right mouse button menu."
        "To view the web page of the article, you can open the article internally in a tab or in an external browser window."));

    connect(this, SIGNAL(currentChanged(Q3ListViewItem*)), this, SLOT(slotCurrentChanged(Q3ListViewItem* )));
    connect(this, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(this, SIGNAL(doubleClicked(Q3ListViewItem*, const QPoint&, int)),  this, SLOT(slotDoubleClicked(Q3ListViewItem*, const QPoint&, int)) );
    connect(this, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
            this, SLOT(slotContextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)));

    connect(this, SIGNAL(mouseButtonPressed(int, Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotMouseButtonPressed(int, Q3ListViewItem *, const QPoint &, int)));
}

Article ArticleListView::currentArticle() const
{
    ArticleItem* ci = dynamic_cast<ArticleItem*>(K3ListView::currentItem());
    return (ci && !selectedItems().isEmpty()) ? ci->article() : Article();
}

void ArticleListView::slotSetFilter(const Akregator::Filters::ArticleMatcher& textFilter, const Akregator::Filters::ArticleMatcher& statusFilter)
{
    if ( (textFilter != d->textFilter) || (statusFilter != d->statusFilter) )
    {
        d->textFilter = textFilter;
        d->statusFilter = statusFilter;

        applyFilters();
    }
}
void ArticleListView::slotPaletteOrFontChanged()
{
    triggerUpdate();
}
        
void ArticleListView::slotShowNode(TreeNode* node)
{
    if (node == d->node)
        return;

    slotClear();

    if (!node)
        return;

    d->node = node;
    connectToNode(node);

    d->columnLayoutVisitor->visit(node);

    setUpdatesEnabled(false);

    QList<Article> articles = d->node->articles();

    QList<Article>::ConstIterator end = articles.end();
    QList<Article>::ConstIterator it = articles.begin();

    for (; it != end; ++it)
    {
        if (!(*it).isNull() && !(*it).isDeleted())
        {
            ArticleItem* ali = new ArticleItem(this, *it);
            d->articleMap.insert((*it).guid(), ali);
        }
    }

    sort();
    applyFilters();
    d->noneSelected = true;
    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotClear()
{
    if (d->node)
        disconnectFromNode(d->node);

    d->node = 0;
    d->articleMap.clear();
    clear();
}

void ArticleListView::slotArticlesAdded(TreeNode* /*node*/, const QList<Article>& list)
{
    setUpdatesEnabled(false);

    for (QList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if (!d->articleMap.contains((*it).guid()))
        {
            if (!(*it).isNull() && !(*it).isDeleted())
            {
                ArticleItem* ali = new ArticleItem(this, *it);
                ali->setVisible( d->textFilter.matches( ali->article()) );
                d->articleMap.insert((*it).guid(), ali);
            }
        }
    }
    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotArticlesUpdated(TreeNode* /*node*/, const QList<Article>& list)
{
    setUpdatesEnabled(false);
    // if only one item is selected and this selected item
    // is deleted, we will select the next item in the list
    bool singleSelected = selectedArticles().count() == 1;

    Q3ListViewItem* next = 0; // the item to select if a selected item is deleted

    for (QList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {

        if (!(*it).isNull() && d->articleMap.contains((*it).guid()))
        {
            ArticleItem* ali = d->articleMap[(*it).guid()];

            if (ali)
            {
                if ((*it).isDeleted()) // if article was set to deleted, delete item
                {
                    if (singleSelected && ali->isSelected())
                    {
                        if (ali->itemBelow())
                            next = ali->itemBelow();
                        else if (ali->itemAbove())
                            next = ali->itemAbove();
                    }
                
                    d->articleMap.remove((*it).guid());
                    delete ali;
                }
                else
                {
                    ali->updateItem(*it);
                    // if the updated article matches the filters after the update, make visible. If it matched them before but not after update, they should stay visible (to not confuse users)
                    if (d->textFilter.matches( ali->article()) && d->statusFilter.matches(ali->article()))
                        ali->setVisible(true);
                }
            } // if ali
        }
    }

    // if the only selected item was deleted, select
    // an item next to it
    if (singleSelected && next != 0)
    {
        setSelected(next, true);
        setCurrentItem(next);
    }
    else
    {
        d->noneSelected = true;
    }

    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::slotArticlesRemoved(TreeNode* /*node*/, const QList<Article>& list)
{
    // if only one item is selected and this selected item
    // is deleted, we will select the next item in the list
    bool singleSelected = selectedArticles().count() == 1;
   
    Q3ListViewItem* next = 0; // the item to select if a selected item is deleted

    setUpdatesEnabled(false);
    
    for (QList<Article>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if (d->articleMap.contains((*it).guid()))
        {
            ArticleItem* ali = d->articleMap[(*it).guid()];
            d->articleMap.remove((*it).guid());
            
            if (singleSelected && ali->isSelected())
            {
                if (ali->itemBelow())
                    next = ali->itemBelow();
                else if (ali->itemAbove())
                    next = ali->itemAbove();
            }

            delete ali;
        }
    }
    
    // if the only selected item was deleted, select
    // an item next to it
    if (singleSelected && next != 0)
    {
        setSelected(next, true);
        setCurrentItem(next);
    }
    else
    {
        d->noneSelected = true;
    }

    setUpdatesEnabled(true);
    triggerUpdate();
}

void ArticleListView::connectToNode(TreeNode* node)
{
    connect(node, SIGNAL(signalDestroyed(TreeNode*)), this, SLOT(slotClear()) );
    connect(node, SIGNAL(signalArticlesAdded(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesAdded(TreeNode*, const QList<Article>&)) );
    connect(node, SIGNAL(signalArticlesUpdated(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesUpdated(TreeNode*, const QList<Article>&)) );
    connect(node, SIGNAL(signalArticlesRemoved(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesRemoved(TreeNode*, const QList<Article>&)) );
}

void ArticleListView::disconnectFromNode(TreeNode* node)
{
    disconnect(node, SIGNAL(signalDestroyed(TreeNode*)), this, SLOT(slotClear()) );
    disconnect(node, SIGNAL(signalArticlesAdded(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesAdded(TreeNode*, const QList<Article>&)) );
    disconnect(node, SIGNAL(signalArticlesUpdated(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesUpdated(TreeNode*, const QList<Article>&)) );
    disconnect(node, SIGNAL(signalArticlesRemoved(TreeNode*, const QList<Article>&)), this, SLOT(slotArticlesRemoved(TreeNode*, const QList<Article>&)) );
}

void ArticleListView::applyFilters()
{
    ArticleItem* ali = 0;
    for (Q3ListViewItemIterator it(this); it.current(); ++it)
    {
        ali = static_cast<ArticleItem*> (it.current());
        ali->setVisible( d->statusFilter.matches( ali->article() ) && d->textFilter.matches( ali->article() ) );
    }
}

int ArticleListView::visibleArticles()
{
    int visible = 0;
    ArticleItem* ali = 0;
    for (Q3ListViewItemIterator it(this); it.current(); ++it) {
        ali = static_cast<ArticleItem*> (it.current());
        visible += ali->isVisible() ? 1 : 0;
    }
    return visible;
}

// from amarok :)
void ArticleListView::paintInfoBox(const QString &message)
{
    QPainter p( viewport() );
    Q3SimpleRichText t( message, QApplication::font() );

    if ( t.width()+30 >= viewport()->width() || t.height()+30 >= viewport()->height() )
            //too big, giving up
        return;

    const uint w = t.width();
    const uint h = t.height();
    const uint x = (viewport()->width() - w - 30) / 2 ;
    const uint y = (viewport()->height() - h - 30) / 2 ;

    p.setBrush( palette().brush( QPalette::Background ) );
    p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
    t.draw( &p, x+15, y+15, QRect(), QColorGroup(palette()) );
}

void ArticleListView::viewportPaintEvent(QPaintEvent *e)
{

    K3ListView::viewportPaintEvent(e);

    if(!e)
        return;

    QString message;

    //kDebug() << "visible articles: " << visibleArticles() << endl;

    if(childCount() != 0) // article list is not empty
    {
        if (visibleArticles() == 0)
        {
        message = i18n("<div align=center>"
                        "<h3>No matches</h3>"
                        "Filter does not match any articles, "
                        "please change your criteria and try again."
                        "</div>");
        }

    }
    else // article list is empty
    {
        if (!d->node) // no node selected
        {
            message = i18n("<div align=center>"
                       "<h3>No feed selected</h3>"
                       "This area is article list. "
                       "Select a feed from the feed list "
                       "and you will see its articles here."
                       "</div>");
        }
        else // empty node
        {
            // TODO: we could display message like "empty node, choose "fetch" to update it"
        }
    }

    if (!message.isNull())
        paintInfoBox(message);
}

Q3DragObject *ArticleListView::dragObject()
{
    Q3DragObject* d = 0;
    QList<Article> articles = selectedArticles();
    if (!articles.isEmpty())
    {
        d = new ArticleDrag(articles, this);
    }
    return d;
}

void ArticleListView::slotPreviousArticle()
{
    ArticleItem* ali = 0;
    if (!currentItem() || selectedItems().isEmpty())
        ali = dynamic_cast<ArticleItem*>(lastChild());
    else
        ali = dynamic_cast<ArticleItem*>(currentItem()->itemAbove());

    if (ali)
    {
        Article a = ali->article();
        setCurrentItem(d->articleMap[a.guid()]);
        clearSelection();
        setSelected(d->articleMap[a.guid()], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotNextArticle()
{
    ArticleItem* ali = 0;
    if (!currentItem() || selectedItems().isEmpty())
        ali = dynamic_cast<ArticleItem*>(firstChild());
    else
        ali = dynamic_cast<ArticleItem*>(currentItem()->itemBelow());

    if (ali)
    {
        Article a = ali->article();
        setCurrentItem(d->articleMap[a.guid()]);
        clearSelection();
        setSelected(d->articleMap[a.guid()], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotNextUnreadArticle()
{
    ArticleItem* start = 0;
    if (!currentItem() || selectedItems().isEmpty())
        start = dynamic_cast<ArticleItem*>(firstChild());
    else
        start = dynamic_cast<ArticleItem*>(currentItem()->itemBelow() ? currentItem()->itemBelow() : firstChild());

    ArticleItem* i = start;
    ArticleItem* unread = 0;

    do
    {
        if (i && i->article().status() != Article::Read)
            unread = i;
        else
            i = dynamic_cast<ArticleItem*>(i && i->itemBelow() ? i->itemBelow() : firstChild());
    }
    while (!unread && i != start);

    if (unread)
    {
        Article a = unread->article();
        setCurrentItem(d->articleMap[a.guid()]);
        clearSelection();
        setSelected(d->articleMap[a.guid()], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::slotPreviousUnreadArticle()
{
    ArticleItem* start = 0;
    if (!currentItem() || selectedItems().isEmpty())
        start = dynamic_cast<ArticleItem*>(lastChild());
    else
        start = dynamic_cast<ArticleItem*>(currentItem()->itemAbove() ? currentItem()->itemAbove() : firstChild());

    ArticleItem* i = start;
    ArticleItem* unread = 0;

    do
    {
        if (i && i->article().status() != Article::Read)
            unread = i;
        else
            i = dynamic_cast<ArticleItem*>(i && i->itemAbove() ? i->itemAbove() : lastChild());
    }
    while ( !(unread != 0 || i == start) );

    if (unread)
    {
        Article a = unread->article();
        setCurrentItem(d->articleMap[a.guid()]);
        clearSelection();
        setSelected(d->articleMap[a.guid()], true);
        d->ensureCurrentItemVisible();
    }
}

void ArticleListView::keyPressEvent(QKeyEvent* e)
{
    e->ignore();
}

void ArticleListView::slotSelectionChanged()
{
    // if there is only one article in the list, currentItem is set initially to
    // that article item, although the user hasn't selected it. If the user selects
    // the article, selection changes, but currentItem does not.
    // executed. So we have to handle this case by observing selection changes.

    if (d->noneSelected)
    {
        d->noneSelected = false;
        slotCurrentChanged(currentItem());
    }
}

void ArticleListView::slotCurrentChanged(Q3ListViewItem* item)
{
    ArticleItem* ai = dynamic_cast<ArticleItem*> (item);
    if (ai)
        emit signalArticleChosen( ai->article() );
    else
    {
        d->noneSelected = true;
        emit signalArticleChosen( Article() );
    }
}


void ArticleListView::slotDoubleClicked(Q3ListViewItem* item, const QPoint& p, int i)
{
    ArticleItem* ali = dynamic_cast<ArticleItem*>(item);
    if (ali)
        emit signalDoubleClicked(ali->article(), p, i);
}

void ArticleListView::slotContextMenu(K3ListView* /*list*/, Q3ListViewItem* /*item*/, const QPoint& p)
{
    QWidget* w = ActionManager::getInstance()->container("article_popup");
    QMenu* popup = static_cast<QMenu *>(w);
    if (popup)
        popup->exec(p);
}

void ArticleListView::slotMouseButtonPressed(int button, Q3ListViewItem* item, const QPoint& p, int column)
{
    ArticleItem* ali = dynamic_cast<ArticleItem*>(item);
    if (ali)
        emit signalMouseButtonPressed(button, ali->article(), p, column);
}

ArticleListView::~ArticleListView()
{
    Settings::setTitleWidth(columnWidth(0));
    Settings::setFeedWidth(columnWidth(1) > 0 ? columnWidth(1) : d->feedWidth);
    Settings::setSortColumn(sortColumn());
    Settings::setSortAscending(sortOrder() == Qt::Ascending);
    Settings::writeConfig();
    delete d->columnLayoutVisitor;
    delete d;
    d = 0;
}

QList<Article> ArticleListView::selectedArticles() const
{
    QList<Article> ret;
    QList<Q3ListViewItem*> items = selectedItems(false);
    Q_FOREACH ( Q3ListViewItem* i, items )
        ret.append((static_cast<ArticleItem*>(i))->article());
    return ret;
}

} // namespace Akregator

#include "articlelistview.moc"
