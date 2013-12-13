/*  Copyright (c) 2013, Abdullah A. Hassan <voodooattack@hotmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "curveeditor.hpp"
#include <mapgenerator.hpp>

#include <QMouseEvent>
#include <QPainter>
#include <QSizeGrip>
#include <QLayout>

namespace ADWIF
{
  CurveEditor::CurveEditor(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f), myHighlightFlag(false), myZoomLevel(1.0)
  {
    QPoint postl = dynamic_cast<QWidget*>(parent->parent())->mapToGlobal(parent->geometry().bottomLeft());
    QRect pos(postl, QSize(parent->geometry().width(), 80));
    setGeometry(pos);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowFlags(Qt::WindowType::FramelessWindowHint | Qt::WindowType::Window));
    setFocusPolicy(Qt::FocusPolicy::WheelFocus);
    setFocus();
    setStyleSheet(QLatin1String(".QWidget {border-style: inset;border-width: 2px; }"));
    setMouseTracking(true);
    myTransform.scale(geometry().width(), geometry().height());
    myTransform.translate(0.5, 0.5);
    QVBoxLayout * layout = new QVBoxLayout(this);
    QSizeGrip * sizeGrip = new QSizeGrip(this);
    layout->setContentsMargins(QMargins(0, 0, 0, 0));
    layout->setSpacing(0);
    layout->addWidget(sizeGrip, 0, Qt::AlignBottom | Qt::AlignRight);
    setLayout(layout);
    myViewport = geometry();
  }

  CurveEditor::~CurveEditor()
  {
  }

  void CurveEditor::mouseMoveEvent(QMouseEvent * e)
  {
    QWidget::mouseMoveEvent(e);
    for (auto const & p : myPoints)
    {
      if (qAbs(myTransform.map(p).x() - e->x()) < 4 && qAbs(myTransform.map(p).y() - e->y()) < 4)
      {
        myHighlight = p;
        myHighlightFlag = true;
        repaint();
        return;
      }
    }
    myHighlightFlag = false;
  }

  void CurveEditor::mousePressEvent(QMouseEvent * e)
  {
    QWidget::mousePressEvent(e);
  }

  void CurveEditor::mouseReleaseEvent(QMouseEvent * e)
  {
    QWidget::mouseReleaseEvent(e);
    if (!myHighlightFlag)
    {
      myPoints << (e->posF() * myTransform.inverted().scale(myZoomLevel, myZoomLevel));
      updateCurve();
      emit curveChanged(myPoints);
    }
    repaint();
  }

  void CurveEditor::focusOutEvent(QFocusEvent * e)
  {
    QWidget::focusOutEvent(e);
    this->close();
  }

  void CurveEditor::wheelEvent(QWheelEvent * e)
  {
    QWidget::wheelEvent(e);
//     if (e->delta() > 0)
//       myZoomLevel += 0.01;
//     else
//       myZoomLevel -= 0.01;
//     myViewport = QTransform::fromScale(myZoomLevel, myZoomLevel).mapRect(geometry());
//     updateCurve();
//     repaint();
  }

  void CurveEditor::resizeEvent(QResizeEvent * e)
  {
    QWidget::resizeEvent(e);
    myTransform.reset();
    myTransform.scale(e->size().width(), e->size().height());
    myTransform.translate(0.5, 0.5);
    myViewport = QTransform::fromScale(myZoomLevel, myZoomLevel).mapRect(geometry());
    updateCurve();
    update();
  }

  void CurveEditor::paintEvent(QPaintEvent * e)
  {
    QWidget::paintEvent(e);
    QPainter painter(this);
    QBrush brush;
    QPen pen;
    painter.setViewport(geometry());
    painter.setWindow(myViewport);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);
    pen.setCapStyle(Qt::PenCapStyle::RoundCap);
    pen.setJoinStyle(Qt::PenJoinStyle::RoundJoin);
    pen.setStyle(Qt::PenStyle::DotLine);
    painter.setPen(pen);
    brush.setColor(Qt::white);
    brush.setStyle(Qt::BrushStyle::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(0,0, myViewport.width()-1, myViewport.height()-1);
    brush.setStyle(Qt::BrushStyle::CrossPattern);
    brush.setColor(Qt::lightGray);
    painter.setBrush(brush);
    painter.drawRect(0,0, geometry().width()-1, geometry().height()-1);
    pen.setColor(Qt::darkGray);
    pen.setStyle(Qt::PenStyle::SolidLine);
    pen.setWidthF(0.5);
    painter.setPen(pen);
    painter.drawLine(0, geometry().height() / 2, geometry().width(), geometry().height() / 2);
    painter.drawLine(geometry().width() / 2, 0, geometry().width() / 2, geometry().height());
    pen.setStyle(Qt::PenStyle::SolidLine);
    pen.setColor(Qt::red);
    pen.setWidthF(2);
    painter.setPen(pen);
    painter.drawPolyline(myPolyline);
    pen.setColor(Qt::black);
    pen.setWidthF(3);
    painter.setPen(pen);
    QPolygonF poly = myTransform.map(myPoints);
    painter.drawPoints(poly);
    if (myHighlightFlag)
    {
      brush.setColor(Qt::transparent);
      pen.setColor(Qt::blue);
      pen.setWidth(1);
      painter.drawEllipse(myHighlight, 3, 3);
    }
  }

  template<typename T> struct PointLess {
    bool operator()(const T & a, const T & b) const { return a.x() < b.x();}
  };

  void CurveEditor::updateCurve()
  {
    qSort(myPoints.begin(), myPoints.end(), PointLess<QPointF>());
    myPolyline.clear();
    QPolygonF points = myTransform.map(myPoints);
    if (myPoints.size() >= 4)
    {
      for (int i = 0; i < geometry().width(); i++)
      {
        double p[4];

        int indexPos;
        for (indexPos = 0; indexPos < points.size(); indexPos++)
          if (i < points[indexPos].x()) break;

        auto clamp = [](int x, int min, int max)
        {
          if (x < min) return min;
          if (x > max) return max;
          return x;
        };

        int i0 = clamp(indexPos - 2, 0, points.size() - 1);
        int i1 = clamp(indexPos - 1, 0, points.size() - 1);
        int i2 = clamp(indexPos    , 0, points.size() - 1);
        int i3 = clamp(indexPos + 1, 0, points.size() - 1);

        if (i1 == i2)
        {
          myPolyline << points[i1];
          continue;
        }

        double in0 = points[i1].x();
        double in1 = points[i2].x();
        double alpha = (i - in0) / (in1 - in0);

        p[0] = points[i0].y();
        p[1] = points[i1].y();
        p[2] = points[i2].y();
        p[3] = points[i3].y();

        myPolyline << QPointF(i, cubicInterpolate(p, alpha));
      }
    }
    else
      myPolyline = points;
  }
}

