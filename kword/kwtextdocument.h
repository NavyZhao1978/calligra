/* This file is part of the KDE project
   Copyright (C) 2001 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kwtextdocument_h
#define kwtextdocument_h

#include "kotextdocument.h"

class KWTextFrameSet;
class KoTextFormatCollection;

/**
 * This is our QTextDocument reimplementation, to create KWTextParag instead of QTextParags,
 * and to relate it to the text frameset it's in.
 */
class KWTextDocument : public KoTextDocument
{
    Q_OBJECT
public:
    /** A real text document inside a frameset */
    KWTextDocument( KWTextFrameSet * textfs, KoTextFormatCollection *fc, KoTextFormatter *formatter = 0L );
    /** A standalone text document, for a preview */
    KWTextDocument( KoZoomHandler * zoomHandler );

    ~KWTextDocument();

    virtual Qt3::QTextParag * createParag( QTextDocument *d, Qt3::QTextParag *pr = 0, Qt3::QTextParag *nx = 0, bool updateIds = TRUE );

    /** Return the text frameset in which this document is.
     * Note that this can be 0L (e.g. for paragraphs in the paragdia preview) */
    KWTextFrameSet * textFrameSet() const { return m_textfs; }

protected:
    void init();
private:
    KWTextFrameSet * m_textfs;
};

#endif
