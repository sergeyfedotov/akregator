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

#ifndef AKREGATOR_ARTICLE_H
#define AKREGATOR_ARTICLE_H

#include "akregator_export.h"
#include "types.h"

#include <boost/shared_ptr.hpp>

class QDateTime;
class QString;

template <class T> class QList;

typedef unsigned int uint;

class KUrl;

namespace Syndication
{
    class Item;
    typedef boost::shared_ptr<Item> ItemPtr;
}

namespace Akregator {

namespace Backend
{
    class FeedStorage;
}
class Feed;
/** A proxy class for Syndication::ItemPtr with some additional methods to assist sorting. */
class AKREGATORPART_EXPORT Article
{
    public:

        Article();
        /** creates am article object for an existing article.
            The constructor accesses the archive to load it's data
            */
        Article(const QString& guid, Feed* feed);
        /** creates an article object from a parsed librss Article
            the article is added to the archive if not yet stored, or updated if stored but modified
        */
        Article(Syndication::ItemPtr article, Feed* feed);
        
        Article(Syndication::ItemPtr article, Backend::FeedStorage* archive);
        Article(const Article &other);
        ~Article();

        Article &operator=(const Article &other);
        bool operator==(const Article &other) const;
        bool operator!=(const Article &other) const;
        

        bool isNull() const;
        
        int status() const;
        void setStatus(int s);

        QString title() const;
        KUrl link() const;
        QString description() const;
        QString guid() const;
        /** if true, the article should be kept even when expired **/
        bool keep() const;
        void setKeep(bool keep);
        bool isDeleted() const;
        
        void setDeleted();

        void offsetPubDate(int secs);

        Feed* feed() const;

        /** returns a hash value used to detect changes in articles with non-hash GUIDs. If the guid is a hash itself, it returns @c 0 */
        
        uint hash() const;

        /** returns if the guid is a hash or an ID taken from the source */
        
        bool guidIsHash() const;
        
        bool guidIsPermaLink() const;
        
        const QDateTime& pubDate() const;
        
        KUrl commentsLink() const;
        
        int comments() const;

        QString author() const;
        
        bool operator<(const Article &other) const;
        bool operator<=(const Article &other) const;
        bool operator>(const Article &other) const;
        bool operator>=(const Article &other) const;


    private:
        void initialize(Syndication::ItemPtr item, Backend::FeedStorage* archive);
        static QString buildTitle(const QString& description);

    private:
        struct Private;
        boost::shared_ptr<Private> d;
};

} // namespace Akregator

#endif // AKREGATOR_ARTICLE_H