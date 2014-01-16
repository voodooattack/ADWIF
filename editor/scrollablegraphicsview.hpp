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

#ifndef SCROLLABLEGRAPHICSVIEW_H
#define SCROLLABLEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QLineEdit>

namespace ADWIF
{
  class ScrollableGraphicsView: public QGraphicsView
  {
    Q_OBJECT
  public:
    ScrollableGraphicsView(QWidget * parent = 0);
    virtual ~ScrollableGraphicsView() { }

    QSizeF cellSize() const { return myCellSize; }
    void setCellSize(const QSizeF & cellSize) { myCellSize = cellSize; }

  protected:
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void wheelEvent(QWheelEvent *);
    virtual void resizeEvent(QResizeEvent *);
  protected slots:
    virtual void viewChanged(const QRectF &);
    virtual void textEdited(const QString &);
  signals:
    void onViewChanged(const QRectF &);
  private:
    QPoint myLastPos;
    QSizeF myCellSize;
    double myScale;
    QLineEdit * myLineEditX, * myLineEditY;
    QValidator * myValidator;
  };
}

#endif // SCROLLABLEGRAPHICSVIEW_H
