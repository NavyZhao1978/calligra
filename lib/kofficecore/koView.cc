/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <koView.h>
#include <koDocument.h>
#include <koMainWindow.h>
#include <koFrame.h>

#include <kinstance.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kparts/plugin.h>
#include <kparts/event.h>
#include <cassert>

class KoViewPrivate
{
public:
  KoViewPrivate()
  {
    m_zoom = 1.0;
    m_children.setAutoDelete( true );
    m_manager = 0L;
    m_tempActiveWidget = 0L;
  }
  ~KoViewPrivate()
  {
  }

  KoDocument *m_doc;

  QGuardedPtr<KParts::PartManager> m_manager;

  double m_zoom;

  QList<KoViewChild> m_children;

  QWidget *m_tempActiveWidget;
};

KoView::KoView( KoDocument *document, QWidget *parent, const char *name )
 : QWidget( parent, name )
{
  ASSERT( document );

  //kdDebug(30003) << "KoView::KoView " << this << endl;
  d = new KoViewPrivate;
  d->m_doc = document;
  PartBase::setPartObject( this );

  setFocusPolicy( StrongFocus );

  setMouseTracking( true );

  connect( d->m_doc, SIGNAL( childChanged( KoDocumentChild * ) ),
	   this, SLOT( slotChildChanged( KoDocumentChild * ) ) );

  setupGlobalActions();
  QValueList<KAction*> docActions = document->actionCollection()->actions();
  QValueList<KAction*>::ConstIterator it = docActions.begin();
  QValueList<KAction*>::ConstIterator end = docActions.end();
  for (; it != end; ++it )
    actionCollection()->insert( *it );
}

KoView::~KoView()
{
  kdDebug(30003) << "KoView::~KoView " << this << endl;
  if ( !koDocument()->isSingleViewMode() )
  {
    if ( d->m_manager )
      d->m_manager->removePart( koDocument() );
    d->m_doc->removeView(this);
  }
  delete d;
  kdDebug(30003) << "leaving KoView::~KoView()" << endl;
}

KoDocument *KoView::koDocument() const
{
  return d->m_doc;
}

bool KoView::hasDocumentInWindow( KoDocument *doc )
{
  return child( doc ) != 0L;
}

void KoView::setPartManager( KParts::PartManager *manager )
{
  d->m_manager = manager;
  if ( !koDocument()->isSingleViewMode() )
    d->m_manager->addPart( koDocument(), false );
}

KParts::PartManager *KoView::partManager() const
{
  return d->m_manager;
}

KAction *KoView::action( const QDomElement &element ) const
{
  static const QString &attrName = KGlobal::staticQString( "name" );
  QString name = element.attribute( attrName );

  KAction *act = KXMLGUIClient::action( name.utf8() );

  if ( !act )
    act = d->m_doc->KXMLGUIClient::action( name.utf8() );

  return act;
}

KoDocument *KoView::hitTest( const QPoint &pos )
{
/*
  if ( selectedChild() && selectedChild()->frameRegion( matrix() ).contains( pos ) )
    return 0L;

  if ( activeChild() && activeChild()->frameRegion( matrix() ).contains( pos ) )
    return 0L;
*/

  KoViewChild *viewChild;
  
  KoDocumentChild *docChild = selectedChild();
  if ( docChild )
  {
    if ( ( viewChild = child( docChild->document() ) ) )
    {
      if ( viewChild->frameRegion( matrix() ).contains( pos ) )
        return 0;
    }
    else
      if ( docChild->frameRegion( matrix() ).contains( pos ) )
        return 0;
  }

  docChild = activeChild();
  if ( docChild )
  {
    if ( ( viewChild = child( docChild->document() ) ) )
    {
      if ( viewChild->frameRegion( matrix() ).contains( pos ) )
        return 0;
    }
    else
      if ( docChild->frameRegion( matrix() ).contains( pos ) )
        return 0;
  }

  return koDocument()->hitTest( QPoint( pos.x() / zoom(),
				        pos.y() / zoom() ) );
}

int KoView::leftBorder() const
{
  return 0;
}

int KoView::rightBorder() const
{
  return 0;
}

int KoView::topBorder() const
{
  return 0;
}

int KoView::bottomBorder() const
{
  return 0;
}

void KoView::setZoom( double zoom )
{
  d->m_zoom = zoom;
  update();
}

double KoView::zoom() const
{
  return d->m_zoom;
}

QWidget *KoView::canvas()
{
  return this;
}

int KoView::canvasXOffset() const
{
  return 0;
}

int KoView::canvasYOffset() const
{
  return 0;
}

void KoView::customEvent( QCustomEvent *ev )
{
  if ( KParts::PartActivateEvent::test( ev ) )
    partActivateEvent( (KParts::PartActivateEvent *)ev );
  else if ( KParts::PartSelectEvent::test( ev ) )
    partSelectEvent( (KParts::PartSelectEvent *)ev );
}

void KoView::partActivateEvent( KParts::PartActivateEvent *event )
{
  if ( event->part() != (KParts::Part *)koDocument() )
  {
    assert( event->part()->inherits( "KoDocument" ) );

    KoDocumentChild *child = koDocument()->child( (KoDocument *)event->part() );
    if ( child && event->activated() )
    {
      if ( child->isRectangle() && !child->isTransparent() )
      {
        KoFrame *frame = new KoFrame( canvas() );
	KoView *view = child->document()->createView( frame );
	view->setXMLGUIBuildDocument( child->document()->viewBuildDocument( view ) );
	
	view->setPartManager( partManager() );
	
	// hack? (Werner)
	view->setZoom( zoom() * QMAX(child->xScaling(), child->yScaling()) );
	
	QRect geom = child->geometry();
	frame->setGeometry( geom.x() * zoom(), geom.y() * zoom(),
			    geom.width() * zoom(), geom.height() * zoom() );
	frame->setView( view );
	frame->show();
	frame->raise();
	KoViewChild *viewChild = new KoViewChild( child, frame );
	/*	
	geom = frame->geometry();
	viewChild->setGeometry( geom );*/
	/*
	viewChild->setGeometry( QRect( geom.x() - view->leftBorder(),
				       geom.y() - view->topBorder(),
				       geom.width() + view->rightBorder(),
				       geom.height() + view->bottomBorder() ) );
	*/
	d->m_children.append( viewChild );
	connect( view, SIGNAL( activated( bool ) ), this, SLOT( slotChildActivated( bool ) ) );
	
	d->m_manager->setActivePart( child->document(), view );
      }
      else
      {
        emit regionInvalidated( child->frameRegion( matrix() ), true );
	emit childActivated( child );
      }
    }
    else if ( child )
    {
      emit regionInvalidated( child->frameRegion( matrix() ), true );
      emit childDeactivated( child );
    }
    else
      emit invalidated();
  }
  else
    emit activated( event->activated() );
}

void KoView::partSelectEvent( KParts::PartSelectEvent *event )
{
  if ( event->part() != (KParts::Part *)koDocument() )
  {
    assert( event->part()->inherits( "KoDocument" ) );
	
    KoDocumentChild *child = koDocument()->child( (KoDocument *)event->part() );

    if ( child && event->selected() )
    {
      emit regionInvalidated( child->frameRegion( matrix() ), true );
      emit childSelected( child );
    }
    else if ( child )
    {
      emit regionInvalidated( child->frameRegion( matrix() ), true );
      emit childUnselected( child );
    }
    else
      emit invalidated();
  }
  else
    emit selected( event->selected() );
}

KoDocumentChild *KoView::selectedChild()
{
  if ( !d->m_manager )
    return 0L;

  KParts::Part *selectedPart = d->m_manager->selectedPart();

  if ( !selectedPart || !selectedPart->inherits( "KoDocument" ) )
    return 0L;

  return koDocument()->child( (KoDocument *)selectedPart );
}

KoDocumentChild *KoView::activeChild()
{
  if ( !d->m_manager )
    return 0L;

  KParts::Part *activePart = d->m_manager->activePart();

  if ( !activePart || !activePart->inherits( "KoDocument" ) )
    return 0L;

  return koDocument()->child( (KoDocument *)activePart );
}

void KoView::paintEverything( QPainter &painter, const QRect &rect, bool transparent )
{
  koDocument()->paintEverything( painter, rect, transparent, this );
}

KoViewChild *KoView::child( KoView *view )
{
  QListIterator<KoViewChild> it( d->m_children );
  for (; it.current(); ++it )
    if ( it.current()->frame()->view() == view )
      return it.current();

  return 0L;
}

KoViewChild *KoView::child( KoDocument *doc )
{
  QListIterator<KoViewChild> it( d->m_children );
  for (; it.current(); ++it )
    if ( it.current()->documentChild()->document() == doc )
      return it.current();

  return 0L;
}

QWMatrix KoView::matrix() const
{
  QWMatrix m;
  m.scale( zoom(), zoom() );
  return m;
}

void KoView::slotChildActivated( bool a )
{
  // Only interested in deactivate events
  if ( a )
    return;

  KoViewChild* ch = child( (KoView*)sender() );
  if ( !ch )
    return;

  KoView* view = ch->frame()->view();

  QWidget *activeWidget = view->d->m_tempActiveWidget;

  if ( d->m_manager->activeWidget() )
    activeWidget = d->m_manager->activeWidget();

  if ( !activeWidget || !activeWidget->inherits( "KoView" ) )
    return;

  // Is the new active view a child of this one ?
  // In this case we may not delete!
  //  QObject *n = d->m_manager->activeWidget();
  QObject *n = activeWidget;
  while( n )
    if ( n == (QObject *)view )
      return;
    else
     n = n->parent();


  d->m_tempActiveWidget = activeWidget;
  d->m_manager->setActivePart( 0L );

  QGuardedPtr<KoDocumentChild> docChild = ch->documentChild();
  QGuardedPtr<KoFrame> chFrame = ch->frame();
  if ( docChild && chFrame && chFrame->view() )
    docChild->document()->setViewBuildDocument( chFrame->view(), chFrame->view()->xmlguiBuildDocument() );

  d->m_children.remove( ch );

  // #### HACK
  // We want to delete as many views as possible and this
  // trick is used to go upwards in the view-tree.
  emit activated( FALSE );
}

void KoView::slotChildChanged( KoDocumentChild *child )
{
  QRegion region( child->oldPointArray( matrix() ) );
  emit regionInvalidated( child->frameRegion( matrix(), true ).unite( region ), true );
}

void KoView::setupGlobalActions() {
    actionNewView = new KAction( i18n( "&New View" ), 0,
        this, SLOT( newView() ),
        actionCollection(), "view_newview" );
}

void KoView::newView() {
    assert( ( d!=0L && d->m_doc != 0L ) );

    KoDocument *thisDocument = d->m_doc;
    KoMainWindow *shell = thisDocument->createShell();
    shell->setRootDocument(thisDocument);
    shell->show();
}


class KoViewChild::KoViewChildPrivate
{
public:
  KoViewChildPrivate()
  {
  }
  ~KoViewChildPrivate()
  {
  }
  bool m_bLock;
};

KoViewChild::KoViewChild( KoDocumentChild *child, KoFrame *frame )
{
  d = new KoViewChildPrivate;
  d->m_bLock = false;
  m_child = child;
  m_frame = frame;
  slotFrameGeometryChanged();
  connect( m_frame, SIGNAL( geometryChanged() ),
	   this, SLOT( slotFrameGeometryChanged() ) );
  connect( m_child, SIGNAL( changed( KoChild * ) ),
	   this, SLOT( slotDocGeometryChanged() ) );
}

KoViewChild::~KoViewChild()
{
  if ( m_frame )
  {
    slotFrameGeometryChanged();
    delete (KoFrame *)m_frame;
  }
  delete d;
}

void KoViewChild::slotFrameGeometryChanged()
{
  QRect geom = m_frame->geometry();
  int b = m_frame->border();
  QRect borderRect( geom.x() + b,
		    geom.y() + b,
		    geom.width() - b * 2,
		    geom.height() - b * 2 );
  QRect borderLessRect( geom.x() + m_frame->leftBorder(),
			geom.y() + m_frame->topBorder(),
			geom.width() - m_frame->leftBorder() - m_frame->rightBorder(),
			geom.height() - m_frame->topBorder() - m_frame->bottomBorder() );
  setGeometry( borderRect );
  if(m_child)
      m_child->setGeometry( borderLessRect );
}

void KoViewChild::slotDocGeometryChanged()
{
  QRect geom = m_child->geometry();
  QRect borderRect( geom.x() - m_frame->leftBorder(),
		    geom.y() - m_frame->topBorder(),
		    geom.width() + m_frame->leftBorder() + m_frame->rightBorder(),
		    geom.height() + m_frame->topBorder() + m_frame->bottomBorder() );
  m_frame->setGeometry( borderRect );
}

#include <koView.moc>
