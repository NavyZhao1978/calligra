/*
 * Kivio - Visual Modelling and Flowcharting
 * Copyright (C) 2000-2003 theKompany.com & Dave Marotti,
 *                         Peter Simonsson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef TOOL_TEXT_H
#define TOOL_TEXT_H

#include "kivio_mousetool.h"

class QMouseEvent;
class QCursor;

class KToggleAction;

class KivioView;
class KivioPage;
class KoPoint;

class TextTool : public Kivio::MouseTool
{
  Q_OBJECT
  public:
    TextTool( KivioView* parent );
    ~TextTool();
  
    virtual void processEvent( QEvent* );
  
    void text(QRect);
  
  public slots:
    virtual void setActivated(bool a);
    virtual void applyToolAction(QPtrList<KivioStencil>* stencils);
    virtual void applyToolAction(KivioStencil* stencil, const KoPoint& pos);
  
  signals:
    void operationDone();
  
  protected:
    void mousePress(QMouseEvent *);
    void mouseMove(QMouseEvent *);
    void mouseRelease(QMouseEvent *);
  
    bool startRubberBanding(QMouseEvent*);
    void continueRubberBanding(QMouseEvent *);
    void endRubberBanding(QMouseEvent *);
  
  
  
    QPoint m_startPoint, m_releasePoint;
  
    // Text Tool Mode
    enum
    {
      stmNone,
      stmDrawRubber
    };
  
  private:
    // Flag to indicate that we are drawing a rubber band
    int m_mode;
    QCursor* m_pTextCursor;
    KToggleAction* m_textAction;
};

#endif


