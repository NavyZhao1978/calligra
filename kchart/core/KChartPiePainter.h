/* $Id$ */

#ifndef _KCHARTPIEPAINTER_H
#define _KCHARTPIEPAINTER_H

#include "KChartPainter.h"

class KChart;
class QPaintDevice;
class QPainter;

class KChartPiePainter : public KChartPainter
{
public:
  KChartPiePainter( KChart* chart );
  ~KChartPiePainter();

  virtual void paintChart( QPaintDevice* paintdev );
  virtual void drawData( QPainter* painter );
  virtual void drawText( QPainter* painter );
  virtual void drawPie( QPainter* painter );

protected:
  const int _angleoffset = 90;
  bool setupCoords( QPaintDevice* );

  int _xcenter;
  int _ycenter;

private:
};

#endif
