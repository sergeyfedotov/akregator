/*
    This file is part of Akregator.

    Copyright (C) 2005 Frank Osterfeld <frank.osterfeld at kdemail.net>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef AKREGATOR_TAGSET_H
#define AKREGATOR_TAGSET_H

#include <qobject.h>

class QDomDocument;
template <class K,class T> class QMap;
class QString;
class QStringList;

namespace Akregator {

class Tag;

class TagSet : public QObject
{
    friend class Tag;
    Q_OBJECT
    public:

        TagSet(QObject* parent=0);
        virtual ~TagSet();

        void insert(const Tag& tag);
        void remove(const Tag& tag);
        QMap<QString,Tag> toMap() const;

        bool contains(const Tag& tag) const;

        Tag findByID(const QString& id) const;

        void readFromXML(const QDomDocument& doc);
        QDomDocument toXML() const;

    signals:
        void signalTagAdded(const Tag&);
        void signalTagRemoved(const Tag&);
        void signalTagUpdated(const Tag&);

    protected:
        void tagUpdated(const Tag& tag);
        
    private:
        class TagSetPrivate;
        TagSetPrivate* d;
};

} // namespace Akregator

#endif // AKREGATOR_TAGSET_H
