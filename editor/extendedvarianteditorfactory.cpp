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

#include "extendedvarianteditorfactory.hpp"
#include "extendedvariantmanager.hpp"
#include "propertybrowserbox.hpp"
#include "curveeditor.hpp"

namespace ADWIF
{
  ExtendedVariantEditorFactory::~ExtendedVariantEditorFactory()
  {

  }

  void ExtendedVariantEditorFactory::connectPropertyManager(QtVariantPropertyManager * manager)
  {
    QObject::connect(manager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                     this, SLOT(onPropertyChanged(QtProperty *, const QVariant &)));
    QObject::connect(manager, SIGNAL(attributeChanged(QtProperty *, const QString &, const QVariant &)),
                     this, SLOT(onPropertyAttributeChanged(QtProperty *, const QString &, const QVariant &)));
    QtVariantEditorFactory::connectPropertyManager(manager);
  }

  void ExtendedVariantEditorFactory::disconnectPropertyManager(QtVariantPropertyManager * manager)
  {
    QObject::disconnect(manager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                        this, SLOT(onPropertyChanged(QtProperty *, const QVariant &)));
    QObject::disconnect(manager, SIGNAL(attributeChanged(QtProperty *, const QString &, const QVariant &)),
                        this, SLOT(onPropertyAttributeChanged(QtProperty *, const QString &, const QVariant &)));
    QtVariantEditorFactory::disconnectPropertyManager(manager);
  }

  QWidget * ExtendedVariantEditorFactory::createEditor(QtVariantPropertyManager * manager, QtProperty * property, QWidget * parent)
  {
    if (manager->propertyType(property) == ExtendedVariantManager::curve2dTypeId())
    {
      PropertyBrowseBox * editor = new PropertyBrowseBox(parent);
      editor->setEditable(false);
      myPropToEditorMap[property].append(editor);
      myEditorToPropMap[editor] = property;
      QObject::connect(editor, SIGNAL(textChanged(QString)),
              this, SLOT(onSetValue(const QString &)));
      QObject::connect(editor, SIGNAL(destroyed(QObject *)),
                       this, SLOT(onEditorDestroyed(QObject *)));
      QObject::connect(editor, SIGNAL(buttonClicked()), this, SLOT(onShowCurveEditor()));
      return editor;
    }
    else if (manager->propertyType(property) == QVariant::Vector3D)
    {
      return nullptr;
    }
    else
      return QtVariantEditorFactory::createEditor(manager, property, parent);
  }

  void ExtendedVariantEditorFactory::onPropertyChanged(QtProperty * property, const QVariant & value)
  {

  }

  void ExtendedVariantEditorFactory::onPropertyAttributeChanged(QtProperty * property, const QString & attribute, const QVariant & value)
  {

  }

  void ExtendedVariantEditorFactory::onSetValue(const QString & value)
  {

  }

  void ExtendedVariantEditorFactory::onEditorDestroyed(QObject * object)
  {

  }

  void ExtendedVariantEditorFactory::onShowCurveEditor()
  {
    PropertyBrowseBox * parent = dynamic_cast<PropertyBrowseBox *>(QObject::sender());
    CurveEditor * editor = new CurveEditor(parent);
    QtProperty * prop = myEditorToPropMap[parent];
    QPolygonF curve = dynamic_cast<QtVariantProperty*>(prop)->value().value<Curve2D>();
    editor->setCurve(curve);
    QObject::connect(editor, SIGNAL(destroyed(QObject *)),
                     this, SLOT(onCurveEditorDestroyed(QObject *)));
    QObject::connect(editor, SIGNAL(curveChanged(const QPolygonF &)), this, SLOT(onCurveChanged(const QPolygonF &)));
    editor->show();
  }

  void ExtendedVariantEditorFactory::onCurveEditorDestroyed(QObject * object)
  {
  }

  void ExtendedVariantEditorFactory::onCurveChanged(const QPolygonF & curve)
  {
    CurveEditor * editor = dynamic_cast<CurveEditor *>(QObject::sender());
    PropertyBrowseBox * parent = dynamic_cast<PropertyBrowseBox *>(editor->parent());

    QList<QtProperty*> props = myEditorToPropMap[parent]->subProperties();
    for (int i = 0; i < props.size(); i++)
      myEditorToPropMap[parent]->removeSubProperty(props[i]);

    QString text;
    for (int i = 0; i < curve.size(); i++)
    {
      QtVariantProperty * subProperty = this->propertyManager(myEditorToPropMap[parent])
        ->addProperty(QVariant::PointF, QString::number(i));
      subProperty->setValue(curve[i]);
      myEditorToPropMap[parent]->addSubProperty(subProperty);
      text += QLatin1String("(") + QString::number(curve[i].x()) + QLatin1String(", ") + QString::number(curve[i].y()) + QLatin1String(")");
      text += QLatin1String(" ");
      mySubpropertyToPropertyMap[subProperty] = myEditorToPropMap[parent];
      QObject::connect(this->propertyManager(myEditorToPropMap[parent]), SIGNAL(propertyChanged(QtProperty*)),
                       this, SLOT(onCurveSubPropertyChanged(QtProperty*)));
    }
    parent->setText(text);
    this->propertyManager(myEditorToPropMap[parent])->setValue(myEditorToPropMap[parent], QVariant::fromValue(curve));
  }

  void ExtendedVariantEditorFactory::onCurveSubPropertyChanged(QtProperty *property)
  {
    QtVariantPropertyManager * manager = dynamic_cast<QtVariantPropertyManager *>(QObject::sender());
    if (QtVariantProperty * parent = manager->variantProperty(mySubpropertyToPropertyMap[property]))
      if (QtVariantProperty * vprop = manager->variantProperty(property))
      {
        Curve2D val = parent->value().value<Curve2D>();
        int index = vprop->propertyName().toInt();
        val[index] = vprop->value().value<QPointF>();
        QVariant var = QVariant::fromValue(val);
        parent->setValue(var);
      }
  }
}

