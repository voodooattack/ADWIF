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

#include "extendedvariantmanager.hpp"

namespace ADWIF
{
  QVariant ExtendedVariantManager::value(const QtProperty * property) const
  {
    if (myCurveData.contains(property))
      return QVariant::fromValue(myCurveData[property].curve);
    else
      return QtVariantPropertyManager::value(property);
  }

  int ExtendedVariantManager::valueType(int propertyType) const
  {
    if (propertyType == curve2dTypeId())
      return curve2dTypeId();
    else
      return QtVariantPropertyManager::valueType(propertyType);
  }

  bool ExtendedVariantManager::isPropertyTypeSupported(int propertyType) const
  {
    if (propertyType == curve2dTypeId())
      return true;
    else
      return QtVariantPropertyManager::isPropertyTypeSupported(propertyType);
  }

  QStringList ExtendedVariantManager::attributes(int propertyType) const
  {
    if (propertyType == curve2dTypeId())
    {
      QStringList attr;
      attr << QLatin1String("maxSize");
      return attr;
    }
    return QtVariantPropertyManager::attributes(propertyType);
  }

  int ExtendedVariantManager::attributeType(int propertyType, const QString & attribute) const
  {
    if (propertyType == curve2dTypeId())
    {
      if (attribute == QLatin1String("maxSize"))
        return QVariant::UInt;
      else
        return 0;
    }
    else
      return QtVariantPropertyManager::attributeType(propertyType, attribute);
  }

  QVariant ExtendedVariantManager::attributeValue(const QtProperty * property, const QString & attribute)
  {
    if (myCurveData.contains(property))
    {
      if (attribute == QLatin1String("maxSize"))
        return myCurveData[property].maxSize;
      else
        return QVariant();
    }
    return QtVariantPropertyManager::attributeValue(property, attribute);
  }

  void ExtendedVariantManager::setValue(QtProperty * property, const QVariant & val)
  {
    if (myCurveData.contains(property))
    {
      if (val.type() != curve2dTypeId() && !val.canConvert((QVariant::Type)curve2dTypeId()))
        return;
      Curve2D v = qVariantValue<Curve2D>(val);
      CurveData2D d = myCurveData[property];
      if (d.curve == v)
        return;
      d.curve = v;
      myCurveData[property] = d;
      emit propertyChanged(property);
      emit valueChanged(property, val);
      return;
    }
    else
      QtVariantPropertyManager::setValue(property, val);
  }

  void ExtendedVariantManager::setAttribute(QtProperty * property, const QString & attribute, const QVariant & value)
  {
    if (myCurveData.contains(property))
    {
      if (attribute == QLatin1String("maxSize"))
      {
        if (value.type() != QVariant::UInt && !value.canConvert(QVariant::UInt))
          return;
        uint v = qVariantValue<uint>(value);
        CurveData2D d = myCurveData[property];
        if (d.maxSize == v)
          return;
        d.maxSize = v;
        myCurveData[property] = d;
        emit attributeChanged(property, attribute, value);
      }
    }
    else
      QtVariantPropertyManager::setAttribute(property, attribute, value);
  }

  QString ExtendedVariantManager::valueText(const QtProperty * property) const
  {
    if (myCurveData.contains(property))
    {
      QString s;
      for (int i = 0; i < myCurveData[property].curve.size(); i++)
      {
        s += QString("[") +
             QString::number(myCurveData[property].curve[i].x()) +
             QString(",") +
             QString::number(myCurveData[property].curve[i].y()) +
             QString("]");
        s += QString(" ");
      }
      return s;
    }
    else
      return QtVariantPropertyManager::valueText(property);
  }

  void ExtendedVariantManager::initializeProperty(QtProperty * property)
  {
    if (propertyType(property) == curve2dTypeId())
      myCurveData[property] = CurveData2D();
    QtVariantPropertyManager::initializeProperty(property);
  }

  void ExtendedVariantManager::uninitializeProperty(QtProperty * property)
  {
    if (propertyType(property) == curve2dTypeId())
      myCurveData.remove(property);
    QtVariantPropertyManager::uninitializeProperty(property);
  }

  int ExtendedVariantManager::curve2dTypeId()
  {
    return qMetaTypeId<Curve2D>();
  }
}

