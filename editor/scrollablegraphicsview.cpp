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

#include "scrollablegraphicsview.hpp"

#include <QMouseEvent>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLayout>
#include <QIntValidator>
#include <qmath.h>
#include <QGLWidget>

namespace ADWIF
{
  ScrollableGraphicsView::ScrollableGraphicsView(QWidget * parent): QGraphicsView(parent), myCellSize(200,200)
  {
    setViewport(new QGLWidget(this));
    setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    myLineEditX = new QLineEdit(this);
    myLineEditY = new QLineEdit(this);
    myValidator = new QIntValidator(this);
    myLineEditX->setValidator(myValidator);
    myLineEditY->setValidator(myValidator);
    QHBoxLayout * hlayout = new QHBoxLayout;
    hlayout->addWidget(myLineEditX);
    hlayout->addWidget(myLineEditY);
    QVBoxLayout * vlayout = new QVBoxLayout(this);
    vlayout->addItem(hlayout);
    vlayout->addStretch();
    setLayout(vlayout);
    QObject::connect(this, SIGNAL(onViewChanged(QRectF)), this, SLOT(viewChanged(QRectF)));
    QObject::connect(myLineEditX, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)));
    QObject::connect(myLineEditY, SIGNAL(textEdited(QString)), this, SLOT(textEdited(QString)));
  }

  void ScrollableGraphicsView::mousePressEvent(QMouseEvent * e)
  {
    QGraphicsView::mousePressEvent(e);
    if (e->button() == Qt::LeftButton)
      myLastPos = e->pos();
  }

  void ScrollableGraphicsView::mouseMoveEvent(QMouseEvent * e)
  {
    QGraphicsView::mouseMoveEvent(e);
    if (e->buttons() & Qt::LeftButton)
    {
      QPoint diff = myLastPos - e->pos();
      myLastPos = e->pos();
      setSceneRect(sceneRect().translated(diff));
      emit onViewChanged(sceneRect());
    }
  }

  void ScrollableGraphicsView::mouseReleaseEvent(QMouseEvent * e)
  {
    QGraphicsView::mouseReleaseEvent(e);
    if (e->button() == Qt::LeftButton)
    {
      myLastPos = e->pos();
      emit onViewChanged(sceneRect());
    }
  }

  void ScrollableGraphicsView::wheelEvent(QWheelEvent * e)
  {
    // ignore all zooming functionality for now, will need to implement multi-res streaming for this
  }

  void ScrollableGraphicsView::resizeEvent(QResizeEvent * e)
  {
    setSceneRect(QRectF((-width() / 2) - myCellSize.width(),
                        (-height() / 2) - myCellSize.height(),
                        width() + myCellSize.width() * 2,
                        height() + myCellSize.height() * 2));
    emit onViewChanged(sceneRect());
  }

  void ScrollableGraphicsView::viewChanged(const QRectF & rect)
  {
    myLineEditX->setText(QString::number(qFloor(rect.center().x())));
    myLineEditY->setText(QString::number(qFloor(rect.center().y())));
  }

  void ScrollableGraphicsView::textEdited(const QString &)
  {
    QLineEdit * edit = dynamic_cast<QLineEdit*>(QObject::sender());
    if (!edit) return;
    QRectF rect = sceneRect();
    rect.moveTo(myLineEditX->text().toDouble() - rect.width() / 2, myLineEditY->text().toDouble() - rect.height() / 2);
    setSceneRect(rect);
    emit onViewChanged(sceneRect());
  }

}

