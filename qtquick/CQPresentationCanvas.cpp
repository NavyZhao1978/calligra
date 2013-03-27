/*
 * This file is part of the KDE project
 *
 * Copyright (C) 2013 Shantanu Tushar <shantanu@kde.org>
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
 *
 */

#include "CQPresentationCanvas.h"
#include "CQCanvasController.h"
#include <KoPart.h>
#include <KoFindText.h>
#include <KoCanvasBase.h>
#include <KoToolManager.h>
#include <KoZoomController.h>
#include <KoZoomHandler.h>
#include <KoPADocument.h>
#include <KoPACanvasItem.h>
#include <KDebug>
#include <KActionCollection>
#include <QGraphicsWidget>

#include <KoPAPageBase.h>

#include <stage/part/KPrDocument.h>

#include "CQPresentationView.h"

class CQPresentationCanvas::Private
{
public:
    Private() : canvasBase(0), view(0) { }

    KoCanvasBase* canvasBase;
    CQPresentationView* view;
};

CQPresentationCanvas::CQPresentationCanvas(QDeclarativeItem* parent)
    : CQCanvasBase(parent), d(new Private)
{

}

CQPresentationCanvas::~CQPresentationCanvas()
{
    delete d;
}

void CQPresentationCanvas::openFile(const QString& uri)
{
    KService::Ptr service = KService::serviceByDesktopName("stagepart");
    if(service.isNull()) {
        qWarning("Unable to load Stage plugin, aborting!");
        return;
    }

    KoPart *part = service->createInstance<KoPart>();
    KPrDocument* document = dynamic_cast<KPrDocument*>(part->document());
    document->setAutoSave(0);
    document->setCheckAutoSaveFile(false);
    document->openUrl (KUrl (uri));

    KoPACanvasItem *paCanvasItem = static_cast<KoPACanvasItem*>(part->canvasItem());
    d->canvasBase = paCanvasItem;
    createAndSetCanvasControllerOn(d->canvasBase);

    d->view = new CQPresentationView(canvasController(), static_cast<KoPACanvasBase*>(d->canvasBase), dynamic_cast<KPrDocument*>(document));
    paCanvasItem->setView(d->view);

    createAndSetZoomController(d->canvasBase);
    d->view->setZoomController(zoomController());
    d->view->connectToZoomController();

    QGraphicsWidget *graphicsWidget = dynamic_cast<QGraphicsWidget*>(d->canvasBase);
    graphicsWidget->setParentItem(this);
    graphicsWidget->installEventFilter(this);
    graphicsWidget->setVisible(true);
    graphicsWidget->setGeometry(x(), y(), width(), height());

    d->view->doUpdateActivePage(document->pageByIndex(0, false));

    //resizeCanvas(QSizeF(width(), height()));
}

void CQPresentationCanvas::createAndSetCanvasControllerOn(KoCanvasBase* canvas)
{
    //TODO: pass a proper action collection
    CQCanvasController *controller = new CQCanvasController(new KActionCollection(this));
    setCanvasController(controller);
    controller->setCanvas(canvas);
    KoToolManager::instance()->addController (controller);
}

void CQPresentationCanvas::createAndSetZoomController(KoCanvasBase* canvas)
{
    KoZoomHandler* zoomHandler = static_cast<KoZoomHandler*> (canvas->viewConverter());
    setZoomController(new KoZoomController(canvasController(),
                                           zoomHandler,
                                           new KActionCollection(this)));

    KoPACanvasItem* canvasItem = static_cast<KoPACanvasItem*>(canvas);

    // update the canvas whenever we scroll, the canvas controller must emit this signal on scrolling/panning
    connect (canvasController()->proxyObject,
                SIGNAL(moveDocumentOffset(QPoint)), canvasItem, SLOT(slotSetDocumentOffset(QPoint)));
    // whenever the size of the document viewed in the canvas changes, inform the zoom controller
    connect (canvasItem, SIGNAL(documentSize(QSize)), this, SLOT(updateDocumentSize(QSize)));
    canvasItem->updateSize();
    canvasItem->update();
}

void CQPresentationCanvas::updateDocumentSize(const QSize& size)
{
    zoomController()->setDocumentSize(d->canvasBase->viewConverter()->viewToDocument(size), false);
}

void CQPresentationCanvas::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    if (d->canvasBase) {
        QGraphicsWidget *widget = dynamic_cast<QGraphicsWidget*>(d->canvasBase);
        if (widget) {
            widget->setGeometry(newGeometry);
        }
    }
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
}
