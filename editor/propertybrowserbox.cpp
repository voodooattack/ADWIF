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

#include "propertybrowserbox.hpp"

#include <QHBoxLayout>
#include <QFocusEvent>
#include <QToolButton>
#include <QLineEdit>

namespace ADWIF
{
  PropertyBrowseBox::PropertyBrowseBox(QWidget * parent, Qt::WindowFlags f): QWidget(parent, f)
  {
    QHBoxLayout * layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    myLineEdit = new QLineEdit(this);
    myLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    myButton = new QToolButton(this);
    myButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
    myButton->setText(QLatin1String("..."));
    layout->addWidget(myLineEdit);
    layout->addWidget(myButton);
    setFocusProxy(myLineEdit);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    connect(myLineEdit, SIGNAL(textEdited(const QString &)),
            this, SIGNAL(textChanged(const QString &)));
    connect(myButton, SIGNAL(clicked()),
            this, SIGNAL(buttonClicked()));
  }
  QString PropertyBrowseBox::text() const { return myLineEdit->text(); }
  void PropertyBrowseBox::setText(const QString & text) { myLineEdit->setText(text); }
  bool PropertyBrowseBox::editable() const { return !myLineEdit->isReadOnly(); }
  void PropertyBrowseBox::setEditable(bool value) { myLineEdit->setReadOnly(!value); }
  void PropertyBrowseBox::focusInEvent(QFocusEvent * e)
  {
    myLineEdit->event(e);
    if (e->reason() == Qt::TabFocusReason ||
        e->reason() == Qt::BacktabFocusReason) { myLineEdit->selectAll(); }
    QWidget::focusInEvent(e);
  }
  void PropertyBrowseBox::focusOutEvent(QFocusEvent * e) { myLineEdit->event(e); QWidget::focusOutEvent(e); }
  void PropertyBrowseBox::keyPressEvent(QKeyEvent * e) { myLineEdit->event(e); }
  void PropertyBrowseBox::keyReleaseEvent(QKeyEvent * e) { myLineEdit->event(e); }
}
