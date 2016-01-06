/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "frame.h"


namespace Akregator
{

class AKREGATOR_EXPORT MainFrame : public Frame
{
    Q_OBJECT

public:

    MainFrame(QWidget *parent, KParts::ReadOnlyPart *part, QWidget *widget);
    virtual ~MainFrame();

    QUrl url() const Q_DECL_OVERRIDE;

    bool openUrl(const OpenUrlRequest &) Q_DECL_OVERRIDE {
        return false;
    }

    KParts::ReadOnlyPart *part() const Q_DECL_OVERRIDE
    {
        return m_part;
    }
private:
    KParts::ReadOnlyPart *m_part;
};

} // namespace Akregator

#endif // MAINFRAME_H