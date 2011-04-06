/* This file is part of the KDE project
 * Copyright (C) 2011 Casper Boemann <cbo@boemann.dk>
 * Copyright (C) 2011 Sebastian Sauer <sebastian.sauer@kdab.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KWROOTAREAPROVIDER_H
#define KWROOTAREAPROVIDER_H

#include <KoTextLayoutRootAreaProvider.h>

class KoShape;
class KoTextShapeData;

class KWRootAreaProvider : public KoTextLayoutRootAreaProvider
{
public:
    KWRootAreaProvider(KoShape *shape, KoTextShapeData *data);

    /// reimplemented
    virtual KoTextLayoutRootArea *provide(KoTextDocumentLayout *documentLayout);
    virtual void releaseAllAfter(KoTextLayoutRootArea *afterThis);

    KoShape *m_shape;
    KoTextShapeData *m_data;
    KoTextLayoutRootArea *m_area;
};

#endif