/* This file is part of the KDE project
 * Copyright (C) 2008 Fela Winkelmolen <fela.kde@gmail.com>
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

#include "KarbonCalligraphyTool.h"
#include "KarbonCalligraphicShape.h"

#include <KoPathShape.h>
#include <KoShapeGroup.h>
#include <KoLineBorder.h>
#include <KoPointerEvent.h>
#include <KoPathPoint.h>
#include <KoCanvasBase.h>
#include <KoShapeController.h>
#include <KoShapeManager.h>
#include <KoSelection.h>
#include <KarbonCurveFit.h>
#include <KoColorBackground.h>

#include <knuminput.h>
#include <klocale.h>
#include <kcombobox.h>

#include <QtGui/QStackedWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QLabel>

#include <cmath>

#undef M_PI
const double M_PI = 3.1415927;
using std::pow;
using std::sqrt;


KarbonCalligraphyTool::KarbonCalligraphyTool(KoCanvasBase *canvas)
    : KoTool( canvas ), m_shape( 0 ), m_strokeWidth( 50 ), m_angle( M_PI/6.0 ),
      m_thinning( 0.0 ), m_mass( 20.0 ), m_isDrawing( false ), m_speed(0, 0)
{
}

KarbonCalligraphyTool::~KarbonCalligraphyTool()
{
}

void KarbonCalligraphyTool::paint( QPainter &painter,
                                   const KoViewConverter &converter )
{
    if ( ! m_shape )
        return;

    painter.save();

    painter.setMatrix( m_shape->absoluteTransformation(&converter) *
                       painter.matrix() );
    m_shape->paint( painter, converter );

    painter.restore();
}

void KarbonCalligraphyTool::mousePressEvent( KoPointerEvent *event )
{
    if ( m_isDrawing ) 
        return;

    m_lastPoint = event->point;
    m_speed = QPointF(0, 0);

    m_isDrawing = true;
    m_shape = new KarbonCalligraphicShape;
    m_shape->setShapeId( KoPathShapeId );
    m_shape->setFillRule( Qt::WindingFill );
    m_shape->setBackground( new KoColorBackground( Qt::black ) );
    m_shape->setBorder( 0 );
    //addPoint( event );
}

void KarbonCalligraphyTool::mouseMoveEvent( KoPointerEvent *event )
{
    if ( ! m_isDrawing )
        return;

    addPoint( event );
}

void KarbonCalligraphyTool::mouseReleaseEvent( KoPointerEvent *event )
{
    Q_UNUSED( event );

    if ( ! m_isDrawing )
        return;

    //addPoint( event );
    m_isDrawing = false;

    if ( m_shape->pointCount() == 0 )
    {
        delete m_shape;
        m_shape = 0;
        return;
    }

    m_shape->simplifyPath();
    KoPathShape *finalPath = m_shape;

    QUndoCommand * cmd = m_canvas->shapeController()->addShape( finalPath );
    if( cmd )
    {
        KoSelection *selection = m_canvas->shapeManager()->selection();
        selection->deselectAll();
        selection->select( finalPath );
        m_canvas->addCommand( cmd );
    }
    else
    {
        // TODO: when may this happen????
        delete finalPath;
    }
    
    m_canvas->updateCanvas( m_shape->boundingRect() );
    m_canvas->updateCanvas( finalPath->boundingRect() );

    m_shape = 0;
}

void KarbonCalligraphyTool::addPoint( KoPointerEvent *event )
{
    QPointF force = event->point - m_lastPoint;

    QPointF dSpeed = force/m_mass;
    m_speed = m_speed/3.0 + dSpeed;

    m_lastPoint = m_lastPoint + m_speed;

    // calculate the modulo of the speed
    double speed = std::sqrt( pow(m_speed.x(), 2) + pow(m_speed.y(), 2) );
    double thinning;
    if ( m_thinning > 0 )
        thinning = m_thinning * (speed + 1) / 10.0;
    else
        thinning = m_thinning * (5 - speed) / 10.0;
    if ( thinning > 1 )
        thinning = 1;

    double strokeWidth = m_strokeWidth * event->pressure() * (1 - thinning);

    const double MINIMUM_STROKE_WIDTH = 1.0;
    if ( strokeWidth < MINIMUM_STROKE_WIDTH )
        strokeWidth = MINIMUM_STROKE_WIDTH;

    m_shape->appendPoint( m_lastPoint, m_angle, strokeWidth );

    m_canvas->updateCanvas( m_shape->lastPieceBoundingRect() );
}



void KarbonCalligraphyTool::activate( bool )
{
    useCursor(Qt::ArrowCursor, true);
}

void KarbonCalligraphyTool::deactivate()
{
}

QWidget *KarbonCalligraphyTool::createOptionWidget()
{
    QWidget *optionWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout( optionWidget );

    QHBoxLayout *widthLayout = new QHBoxLayout( optionWidget );
    QLabel *widthLabel = new QLabel( i18n( "Width" ), optionWidget );
    QDoubleSpinBox *widthBox = new QDoubleSpinBox;
    widthBox->setRange( 0.0, 1000.0 );
    widthBox->setValue( m_strokeWidth );
    widthLayout->addWidget( widthLabel );
    widthLayout->addWidget( widthBox );
    layout->addLayout( widthLayout );

    QHBoxLayout *thinningLayout = new QHBoxLayout( optionWidget );
    QLabel *thinningLabel = new QLabel( i18n( "Thinning" ), optionWidget );
    QDoubleSpinBox *thinningBox = new QDoubleSpinBox;
    thinningBox->setRange( -1.0, 1.0 );
    thinningBox->setSingleStep( 0.1 );
    thinningBox->setValue( 0.0 );
    thinningLayout->addWidget( thinningLabel );
    thinningLayout->addWidget( thinningBox );
    layout->addLayout( thinningLayout );

    QHBoxLayout *angleLayout = new QHBoxLayout( optionWidget );
    QLabel *angleLabel = new QLabel( i18n( "Angle" ), optionWidget );
    QSpinBox *angleBox = new QSpinBox;
    angleBox->setRange( 0, 180 );
    angleBox->setValue( m_angle*180/M_PI );
    angleLayout->addWidget( angleLabel );
    angleLayout->addWidget( angleBox );
    layout->addLayout( angleLayout );

    QHBoxLayout *massLayout = new QHBoxLayout( optionWidget );
    QLabel *massLabel = new QLabel( i18n( "Mass" ), optionWidget );
    QSpinBox *massBox = new QSpinBox;
    massBox->setRange( 1, 20 );
    massBox->setValue( qRound(sqrt(m_mass)) );
    massLayout->addWidget( massLabel );
    massLayout->addWidget( massBox );
    layout->addLayout( massLayout );

    connect( widthBox, SIGNAL(valueChanged(double)),
             this, SLOT(setStrokeWidth(double)));

    connect( thinningBox, SIGNAL(valueChanged(double)),
             this, SLOT(setThinning(double)));

    connect( angleBox, SIGNAL(valueChanged(int)),
             this, SLOT(setAngle(int)));

    connect( massBox, SIGNAL(valueChanged(int)),
             this, SLOT(setMass(int)));

    return optionWidget;
}

void KarbonCalligraphyTool::setStrokeWidth( double width )
{
    m_strokeWidth = width;
}

void KarbonCalligraphyTool::setThinning( double thinning )
{
    m_thinning = thinning/2.0;
}

void KarbonCalligraphyTool::setAngle( int angle )
{
    m_angle = angle/180.0*M_PI;
}

void KarbonCalligraphyTool::setMass( int mass )
{
    m_mass = mass * mass;
}
