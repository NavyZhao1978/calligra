/*
 *  kis_tool.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qevent.h>
#include <qpainter.h>
#include <qpointarray.h>

#include <kdebug.h>
#include <kmessagebox.h>

#include "kis_canvas.h"
#include "kis_cursor.h"
#include "kis_doc.h"
#include "kis_view.h"
#include "kis_tool.h"

KisTool::KisTool(KisDoc *doc, const char *name) //: QObject(doc, name)
{
	m_pDoc = doc;
	m_pView = doc -> currentView();
	assert(m_pView);
	m_Cursor = KisCursor::arrowCursor();
}

KisTool::~KisTool() 
{
}

void KisTool::optionsDialog()
{
	KMessageBox::sorry(0, "Options for current tool... coming soon", "", false);  
}

void KisTool::setChecked(bool /*check*/)
{
}

void KisTool::setCursor(const QCursor& c)
{
	m_Cursor = c;
}

void KisTool::setCursor()
{
	setCursor(arrowCursor);
}

QCursor KisTool::cursor() const
{
	return m_Cursor;
}

// translate integer for zoom factor
int KisTool::zoomed(int n) const
{
	return static_cast<int>(n / m_pView -> zoomFactor());
}

// translate integer for zoom factor
int KisTool::zoomedX(int n) const
{
	// current zoom factor for this view
	float zF = m_pView -> zoomFactor();

	return static_cast<int>(n / zF);
}

// translate integer for zoom factor
int KisTool::zoomedY(int n) const
{
	// current zoom factor for this view
	float zF = m_pView -> zoomFactor();

	return static_cast<int>(n / zF);    
}

// translate point for zoom factor
QPoint KisTool::zoomed(const QPoint & pt) const
{
	// current zoom factor for this view
	float zF = m_pView -> zoomFactor();
   
	// translate startpoint for zoom factor
	// this is almost always a from a mouse event
     
	int startx = static_cast<int>(pt.x() / zF);
	int starty = static_cast<int>(pt.y() / zF);
    
	// just dealing with canvas, no scroll accounting        
	return QPoint(startx, starty);
}

void KisTool::mousePress(QMouseEvent*)
{
}

void KisTool::mouseMove(QMouseEvent*)
{
}

void KisTool::mouseRelease(QMouseEvent*)
{
}

// get QRect for draw freehand in layer.
QRect KisTool::getDrawRect(const QPointArray& points) const
{
	int maxX = 0, maxY = 0;
	int minX = 0, minY = 0;
	int tmpX = 0, tmpY = 0;
	bool first = true;
	QPointArray::ConstIterator it;

	for (it = points.begin(); it != points.end(); it++) {
		const QPoint& pt = *it;

		tmpX = pt.x();
		tmpY = pt.y();

		if (first) {
			maxX = tmpX;
			maxY = tmpY;
			minX = tmpX;
			minY = tmpY;
			first = false;
		}

		if (maxX < tmpX)
			maxX = tmpX;

		if (maxY < tmpY)
			maxY = tmpY;

		if (minX > tmpX)
			minX = tmpX;

		if (minY > tmpY)
			minY = minY;
	}

	QPoint topLeft = QPoint(minX, minY);
	QPoint bottomRight = QPoint(maxX, maxY);
	return QRect(zoomed(topLeft), zoomed(bottomRight));
}

// get QPointArray for draw freehand and polyline in layer.
QPointArray KisTool::zoomPointArray(const QPointArray& points) const
{
	QPointArray rc(points.size());
	int count = 0;
    
	for (QPointArray::ConstIterator it = points.begin(); it != points.end(); it++) {
		rc.setPoint(count, zoomed(*it));
		count++;
	}

	return rc;
}

// clip select area image
void KisTool::setClipImage()
{
    // set select area clip
    if ( !m_pDoc->setClipImage() ) {
        kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): m_pDoc->setClipImage() failed" << endl;
        return;
    }

    // get select area clip
    if ( m_pDoc->getClipImage() ) {
        kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): m_pDoc->getClipImage() success!!" << endl;
        clipImage = *m_pDoc->getClipImage();

        if ( clipImage.isNull() ) {
            kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): clip image is null!" << endl;
            return;
        }
        // if dealing with 1 or 8 bit image, convert to 16 bit
        if ( clipImage.depth() < 16 ) {
            QImage smoothImage = clipImage.smoothScale( clipImage.width(), clipImage.height() );
            clipImage = smoothImage;

            if ( clipImage.isNull() ) {
                kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): smooth scale clip image is null!" << endl;
                return;
            }
        }

        clipPixmap.convertFromImage( clipImage, QPixmap::AutoColor );
        if ( clipPixmap.isNull() ) {
            kdDebug() << "FreehandSelectTool::setClipImage(): can't convert from image!" << endl;
            return;
        }

        if ( !clipImage.hasAlphaBuffer() )
            kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): clip image has no alpha buffer!" << endl;
    }

    kdDebug( 0 ) << "FreehandSelectTool::setClipImage(): Success set up clip image!!" << endl;
}

// drag clip image
void KisTool::dragSelectImage(const QPoint& dragPoint, const QPoint& hotSpot) const
{
    KisImage *img = m_pDoc->current();
    if ( !img )
        return;

    KisLayer *lay = img->getCurrentLayer();
    if ( !lay )
        return;

    float zF = m_pView->zoomFactor();
    int pX = dragPoint.x();
    int pY = dragPoint.y();
    pX = (int)( pX / zF );
    pY = (int)( pY / zF );
    QPoint point = QPoint( pX, pY );

    QPainter p;
    p.begin( m_pView->kisCanvas() );
    p.scale( zF, zF );

    QRect imageRect( point.x() - hotSpot.x(), point.y() - hotSpot.y(), 
                     clipPixmap.width(), clipPixmap.height() );
    imageRect = imageRect.intersect( img->imageExtents() );

    if ( imageRect.top() > img->height() || imageRect.left() > img->width()
         || imageRect.bottom() < 0 || imageRect.right() < 0 ) {
        p.end();
        return;
    }

    if ( !imageRect.intersects( img->imageExtents() ) ) {
        p.end();
        return;
    }

    imageRect = imageRect.intersect( img->imageExtents() );

    int startX = 0;
    int startY = 0;

    if ( clipPixmap.width() > imageRect.right() )
        startX = clipPixmap.width() - imageRect.right();
    if ( clipPixmap.height() > imageRect.bottom() )
        startY = clipPixmap.height() - imageRect.bottom();

    // paranioa
    if( startX < 0 ) 
        startX = 0;
    if( startY < 0 )
        startY = 0;
    if( startX > clipPixmap.width() )
        startX = clipPixmap.width();
    if( startY > clipPixmap.height() )
        startY = clipPixmap.height();

    int xt = m_pView->xPaintOffset() - m_pView->xScrollOffset();
    int yt = m_pView->yPaintOffset() - m_pView->yScrollOffset();

    p.translate( xt, yt );

    p.drawPixmap( imageRect.left(), imageRect.top(),
                  clipPixmap,
                  startX, startY,
                  imageRect.width(), imageRect.height() );

    p.end();
}

// pasete clip image
bool KisTool::pasteClipImage(const QPoint& pos)
{
    KisImage *img = m_pDoc->current();
    if ( !img )
        return false;

    KisLayer *lay = img->getCurrentLayer();
    if ( !lay )
        return false;

    QImage *qimg = &clipImage;

    int startx = pos.x();
    int starty = pos.y();

    QRect clipRect( startx, starty, qimg->width(), qimg->height() );

    if ( !clipRect.intersects( img->getCurrentLayer()->imageExtents() ) )
        return false;

    clipRect = clipRect.intersect( img->getCurrentLayer()->imageExtents() );

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar r, g, b, a;
    int   v = 255;
    int   bv = 0;

    int red     = m_pView->fgColor().R();
    int green   = m_pView->fgColor().G();
    int blue    = m_pView->fgColor().B();

    bool grayscale = false;
    bool colorBlending = false;
    bool layerAlpha = ( img->colorMode() == cm_RGBA );
    bool imageAlpha = qimg->hasAlphaBuffer();

    for ( int y = sy; y <= ey; ++y ) {
        for (int x = sx; x <= ex; ++x) {
            // destination binary values by channel
            r = lay->pixel(0, startx + x, starty + y);
            g = lay->pixel(1, startx + x, starty + y);
            b = lay->pixel(2, startx + x, starty + y);

            // pixel value in scanline at x offset to right
            uint *p = (uint *)qimg->scanLine(y) + x;

            // if the alpha value of the pixel in the selection
            // image is 0, don't paint the pixel.  It's transparent.
            if( ( imageAlpha ) && ( ( (*p) >> 24 ) == 0 ) )
                continue;

            if( colorBlending ) {
                // make mud!
                lay->setPixel( 0, startx + x, starty + y, ( qRed(*p) + r + red ) / 3 );
                lay->setPixel( 1, startx + x, starty + y, ( qGreen(*p) + g + green ) / 3 );
                lay->setPixel( 2, startx + x, starty + y, ( qBlue(*p) + b + blue ) / 3 );
            }
            else {
                // set layer pixel to be same as image
                lay->setPixel( 0, startx + x, starty + y, qRed(*p) );
                lay->setPixel( 1, startx + x, starty + y, qGreen(*p) );
                lay->setPixel( 2, startx + x, starty + y, qBlue(*p) );
            }

            if ( layerAlpha ) {
                a = lay->pixel(3, startx + x, starty + y);
                if( grayscale ) {
                    v = a + bv;
                    if ( v < 0 )
                        v = 0;
                    if ( v > 255 )
                        v = 255;
                    a = (uchar) v;
                }
                else {
                    v = (int)((*p) >> 24);
                    v += a;
                    if (v < 0 ) 
                        v = 0;
                    if (v > 255 )
                        v = 255;
                    a = (uchar) v;
                }

                lay->setPixel( 3, startx + x, starty + y, a );
            }
        }
    }

    return true;
}

bool KisTool::shouldRepaint() const
{
	return false;
}

void KisTool::setBrush(KisBrush *brush)
{
	m_pBrush = brush;
}

void KisTool::clearOld()
{
}

void KisTool::toolSelect()
{
	if (m_pView)
		m_pView -> activateTool(this);
}

void KisTool::setPattern(KisPattern *pattern)
{
	m_pPattern = pattern;
}

bool KisTool::willModify() const
{
	return true;
}

// set select cursor
void KisTool::setSelectCursor()
{
	m_Cursor = KisCursor::selectCursor();

	if (m_pView)
		m_pView -> kisCanvas() -> setCursor(KisCursor::selectCursor());
}

// set move cursor
void KisTool::setMoveCursor()
{
	m_Cursor = KisCursor::moveCursor();

	if (m_pView)
		m_pView -> kisCanvas() -> setCursor(KisCursor::moveCursor());
}

QDomElement KisTool::saveSettings(QDomDocument& /*doc*/) const
{
	return QDomElement();
}

bool KisTool::loadSettings(QDomElement& /*elem*/)
{
	return false;
}

#include "kis_tool.moc"

