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


#ifndef EXTENDEDVARIANTMANAGER_H
#define EXTENDEDVARIANTMANAGER_H

#include <QtVariantProperty>
#include <QtVariantPropertyManager>

#include <QVector2D>
#include <QVector>

typedef QPolygonF Curve2D;

Q_DECLARE_METATYPE(Curve2D);

namespace ADWIF
{
  class ExtendedVariantManager : public QtVariantPropertyManager
  {
    Q_OBJECT
  public:
    ExtendedVariantManager(QObject * parent = 0): QtVariantPropertyManager(parent) { }

    virtual QVariant value(const QtProperty * property) const;
    virtual int valueType(int propertyType) const;
    virtual bool isPropertyTypeSupported(int propertyType) const;

    virtual QStringList attributes(int propertyType) const;
    virtual int attributeType(int propertyType, const QString & attribute) const;
    virtual QVariant attributeValue(const QtProperty * property, const QString & attribute);

    static int curve2dTypeId();

  public slots:
    virtual void setValue(QtProperty * property, const QVariant & val);
    virtual void setAttribute(QtProperty * property,
                              const QString & attribute, const QVariant & value);
  protected:
    virtual QString valueText(const QtProperty * property) const;
    virtual void initializeProperty(QtProperty * property);
    virtual void uninitializeProperty(QtProperty * property);
  private:
    struct CurveData2D
    {
      Curve2D curve;
      uint maxSize;
    };
    QMap<const QtProperty *, CurveData2D> myCurveData;
  };
}

#endif // EXTENDEDVARIANTMANAGER_H
