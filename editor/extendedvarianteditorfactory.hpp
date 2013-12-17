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

#ifndef EXTENDEDVARIANTEDITORFACTORY_H
#define EXTENDEDVARIANTEDITORFACTORY_H

#include <QtVariantEditorFactory>

namespace ADWIF
{
  class ExtendedVariantEditorFactory : public QtVariantEditorFactory
  {
    Q_OBJECT
  public:
    ExtendedVariantEditorFactory(QObject * parent = 0): QtVariantEditorFactory(parent) { }
    virtual ~ExtendedVariantEditorFactory();
  protected:
    virtual void connectPropertyManager(QtVariantPropertyManager * manager);
    virtual QWidget * createEditor(QtVariantPropertyManager * manager, QtProperty * property,
                                   QWidget * parent);
    virtual void disconnectPropertyManager(QtVariantPropertyManager * manager);
  private slots:
    void onPropertyChanged(QtProperty * property, const QVariant & value);
    void onPropertyAttributeChanged(QtProperty * property, const QString & attribute, const QVariant & value);
    void onSetValue(const QString & value);
    void onEditorDestroyed(QObject * object);
    void onCurveEditorDestroyed(QObject * object);
    void onCurveChanged(const QPolygonF & curve);
    void onShowCurveEditor();
    void onCurveSubPropertyChanged(QtProperty * property);
  private:
    QMap<QtProperty *, QList<QWidget*> > myPropToEditorMap;
    QMap<QWidget *, QtProperty *> myEditorToPropMap;
    QMap<QtProperty *, QtProperty *> mySubpropertyToPropertyMap;
  };
}

#endif // EXTENDEDVARIANTEDITORFACTORY_H