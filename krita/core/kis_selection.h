/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */
#ifndef KIS_SELECTION_H_
#define KIS_SELECTION_H_

#include <qrect.h>

#include "kis_types.h"
#include "kis_paint_device.h"

#include <koffice_export.h>


enum enumSelectionMode {
    SELECTION_ADD,
    SELECTION_SUBTRACT
};

/**
 * KisSelection contains a byte-map representation of a layer, where
 * the value of a byte signifies whether a corresponding pixel is selected, or not.
 *
 * NOTE: If you need to manually call emitSelectionChanged on the owner paint device
 *       of a selection. KisSelection does not emit any signals by itself because
 *       often you want to combine several actions in to perfom one operation and you
 *       do not want recomposition to happen all the time.
 */
class KRITACORE_EXPORT KisSelection : public KisPaintDevice {

    typedef KisPaintDevice super;

public:
    /**
     * Create a new KisSelection
    * @param dev the parent paint device. The selection will never be bigger than the parent
     *              paint device.
     */
    KisSelection(KisPaintDeviceSP dev);

    /**
     * Create a new KisSelection. This selection will not have a parent paint device.
     */
    KisSelection();
    
    /**
     * Copy the selection
     */
    KisSelection(const KisSelection& rhs);
    
    virtual ~KisSelection();

    // Returns selectedness, or 0 if invalid coordinates
    quint8 selected(qint32 x, qint32 y);

    void setSelected(qint32 x, qint32 y, quint8 s);

    QImage maskImage();

    void select(QRect r);

    void invert();

    void clear(QRect r);

    void clear();

    // Tests if the the rect is totally outside the selection
    bool isTotallyUnselected(QRect r);

    /**
     * Rough, but fastish way of determining the area
     * of the tiles used by the selection.
     */
    QRect selectedRect();

    /**
     * Slow, but exact way of determining the rectangle
     * that encloses the selection
     */
    QRect selectedExactRect();

    void paintSelection(QImage img, qint32 x, qint32 y, qint32 w, qint32 h);
    void paintSelection(QImage img, const QRect& scaledImageRect, const QSize& scaledImageSize, const QSize& imageSize);

private:
    void paintUniformSelectionRegion(QImage img, const QRect& imageRect, const QRegion& uniformRegion);

private:
    KisPaintDeviceSP m_parentPaintDevice;
};

#endif // KIS_SELECTION_H_
