/* This file is part of the KDE project
   Copyright (C) 2001, 2002, 2003 The Karbon Developers

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


#include "vellipse.h"
#include "vtransformcmd.h"
#include <klocale.h>
#include <vglobal.h>
#include <qdom.h>

VEllipse::VEllipse( VObject* parent, VState state ) : VComposite( parent, state )
{
}

VEllipse::VEllipse( VObject* parent,
		const KoPoint& topLeft, double width, double height,
		VEllipseType type, double startAngle, double endAngle )
	: VComposite( parent ), m_type( type ), m_startAngle( startAngle ), m_endAngle( endAngle )
{
	setDrawCenterNode();

	m_rx = width / 2.0;
	m_ry = height / 2.0;
	m_center.setX( topLeft.x() + m_rx );
	m_center.setY( topLeft.y() + m_ry );

	init();
}

void
VEllipse::init()
{
	// to radials
	uint nsegs = ( m_endAngle - m_startAngle ) / 90.0;
	int i = m_startAngle / 90.0;
	double origEndAngle = m_endAngle;
	double startAngle = VGlobal::pi_2 * ( m_startAngle / 90.0 );
	double endAngle   = VGlobal::pi_2 * ( m_endAngle / 90.0 );
	// Create (half-)unity circle with topLeft at (0|0):
	double currentAngle = -startAngle - VGlobal::pi_2;
	KoPoint start( 0.5 * sin( -startAngle ), 0.5 * cos( -startAngle ) );
	moveTo( KoPoint( start.x(), start.y() ) );
	KoPoint current;
	double midAngle = -startAngle - VGlobal::pi_2 / 2.0;
	while( i < nsegs )
	{
		i = ++i % 4;
		current = KoPoint( 0.5 * sin( currentAngle ), 0.5 * cos( currentAngle ) );
		currentAngle -= VGlobal::pi_2;
		midAngle -= VGlobal::pi_2;
		arcTo( KoPoint( cos( midAngle ) * ( 0.5 / cos( VGlobal::pi_2 / 2.0 ) ),
						-sin( midAngle ) * ( 0.5 / cos( VGlobal::pi_2 / 2.0 ) ) ) , current, 0.5 );
	/*kdDebug() << "ctrl x : " << cos( midAngle ) * ( 0.5 / cos( VGlobal::pi_2 / 2.0 ) ) << endl;
	kdDebug() << "ctrl y : " << -sin( midAngle ) * ( 0.5 / cos( VGlobal::pi_2 / 2.0 ) ) << endl;
	kdDebug() << "current : " << current << endl;
	kdDebug() << "currentAngle : " << currentAngle << endl;
	kdDebug() << "midAngle : " << midAngle << endl;*/
	}
	//currentAngle += VGlobal::pi_2;
	double rest = (int)origEndAngle % 90 + 90;
	midAngle = currentAngle + ( rest / 360.0 ) * VGlobal::pi;
	KoPoint end( 0.5 * sin( -endAngle ), 0.5 * cos( -endAngle ) );
	//arcTo( KoPoint( cos( -midAngle ) * ( 0.5 / cos( rest ) ), -sin( -midAngle ) * ( 0.5 / cos( rest ) ) ), end, 0.5 );
	lineTo( end );
	//arcTo( KoPoint( cos( midAngle ) * ( 0.5 / cos( currentAngle - midAngle ) ),
	//				-sin( midAngle ) * ( 0.5 / cos( currentAngle - midAngle ) ) ), end, 0.5 );
	kdDebug() << "ctrl x : " << cos( midAngle ) * ( 0.5 / cos( currentAngle - midAngle ) ) << endl;
	kdDebug() << "ctrl y : " << -sin( midAngle ) * ( 0.5 / cos( currentAngle - midAngle ) ) << endl;
	kdDebug() << "rest : " << rest << endl;
	kdDebug() << "endAngle : " << endAngle << endl;
	kdDebug() << "currentAngle : " << currentAngle << endl;
	kdDebug() << "midAngle : " << midAngle << endl;
	kdDebug() << "end : " << end << endl;
	if( m_type == cut )
		lineTo( KoPoint( 0.0, 0.0 ) );
	if( m_type != arc )
		close();

	// Translate and scale:
	QWMatrix m;
	m.translate( m_center.x() - m_rx, m_center.y() - m_ry );
	m.scale( 2.0 * m_rx, 2.0 * m_ry );

	VTransformCmd cmd( 0L, m );
	cmd.visit( *this );
}

QString
VEllipse::name() const
{
	QString result = VObject::name();
	return !result.isEmpty() ? result : i18n( "Ellipse" );
}

void
VEllipse::save( QDomElement& element ) const
{
	if( state() != deleted )
	{
		QDomElement me = element.ownerDocument().createElement( "ELLIPSE" );
		element.appendChild( me );

		VObject::save( me );

		me.setAttribute( "cx", m_center.x() );
		me.setAttribute( "cy", m_center.y() );

		me.setAttribute( "rx", m_rx );
		me.setAttribute( "ry", m_ry );

		me.setAttribute( "start-angle", m_startAngle );
		me.setAttribute( "end-angle", m_endAngle );

		if( m_type == cut )
			me.setAttribute( "kind", "cut" );
		else if( m_type == section )
			me.setAttribute( "kind", "section" );
		else if( m_type == arc )
			me.setAttribute( "kind", "arc" );
		else
			me.setAttribute( "kind", "full" );
	}
}

void
VEllipse::load( const QDomElement& element )
{
	setState( normal );

	VObject::load( element );

	m_rx = element.attribute( "rx" ).toDouble(),
	m_ry = element.attribute( "ry" ).toDouble(),

	m_center.setX( element.attribute( "cx" ).toDouble() );
	m_center.setY( element.attribute( "cy" ).toDouble() );

	m_startAngle = element.attribute( "start-angle" ).toDouble();
	m_endAngle = element.attribute( "end-angle" ).toDouble();

	if( element.attribute( "kind" ) == "cut" )
		m_type = cut;
	else if( element.attribute( "kind" ) == "section" )
		m_type = section;
	else if( element.attribute( "kind" ) == "arc" )
		m_type = arc;
	else
		m_type = full;

	init();
}

