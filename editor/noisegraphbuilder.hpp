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

#ifndef NOISEGRAPHBUILDER_H
#define NOISEGRAPHBUILDER_H

#include <QList>

#include <QtGui/QWidget>
#include <QtGui/QTreeView>
#include <QStandardItemModel>
#include <QMenu>

#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <QtTreePropertyBrowser>

#include <json/value.h>

#include "extendedvarianteditorfactory.hpp"
#include "extendedvariantmanager.hpp"

namespace ADWIF
{
  struct ModuleTemplate
  {
    QString name;
    std::string jsonName;
    int sources;
    std::string icon;
    std::function<QList<QtVariantProperty*>(QtVariantPropertyManager&)> addToManager;
    std::function<Json::Value(QtVariantPropertyManager&)> toJson;
  };

  class NoiseModuleItem: public QObject, public QStandardItem
  {
    Q_OBJECT
  public:
    using QStandardItem::parent;

    NoiseModuleItem() { setupPropertyManager(); }
    NoiseModuleItem(const NoiseModuleItem & other): QStandardItem(other) {
      setupPropertyManager();
      setModuleTemplate(other.myTemplate);
    }
    NoiseModuleItem(const QString & text): QStandardItem(text) { setupPropertyManager(); }
    NoiseModuleItem(const QIcon & icon, const QString & text): QStandardItem(icon, text) { setupPropertyManager(); }

    virtual ~NoiseModuleItem() { }

    void setupPropertyManager()
    {
      myProperties.clear();
      myManager = QSharedPointer<ExtendedVariantManager>(new ExtendedVariantManager);
      myFactory = QSharedPointer<ExtendedVariantEditorFactory>(new ExtendedVariantEditorFactory);
      myFactory->addPropertyManager(myManager.data());
      QObject::connect(myManager.data(), SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                       this, SLOT(valueChanged(QtProperty *, const QVariant &)));
    }

    virtual QStandardItem * clone() const
    {
      NoiseModuleItem * newItem = new NoiseModuleItem(*this);
      return newItem;
    }

    const ModuleTemplate & moduleTemplate() const { return myTemplate; }
    void setModuleTemplate(const ModuleTemplate & templ) {
      myTemplate = templ;
      myProperties.clear();
      myManager->clear();
      if (templ.addToManager)
        myProperties = templ.addToManager(*myManager);
    }

    const QString & name() const { return myTemplate.name; }
    void setName(const QString & name) { myTemplate.name = name; }

    void addToPropertyBrowser(QtTreePropertyBrowser * browser)
    {
      for(int i = 0; i < myProperties.size(); i++)
        browser->addProperty(myProperties[i]);
      browser->setFactoryForManager(myManager.data(), myFactory.data());
    }

  public slots:
    void valueChanged(QtProperty *, const QVariant &) { }
  private:
    ModuleTemplate myTemplate;
    QSharedPointer<QtVariantPropertyManager> myManager;
    QSharedPointer<QtVariantEditorFactory> myFactory;
    QList<QtVariantProperty *> myProperties;
  };

  class NoiseGraphBuilder: public QTreeView
  {
    Q_OBJECT
  public:
    explicit NoiseGraphBuilder(QWidget * parent = 0);

    void setPropertyBrowser(QtTreePropertyBrowser * propertyBrowser) { myPropertyBrowser = propertyBrowser; }
    bool isComplete() const;
  private:
    bool isComplete(const QStandardItem * item) const;

  public slots:
    void onShowContextMenu(const QPoint & pt);
    void onActionTriggered();
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  private:
    QtTreePropertyBrowser * myPropertyBrowser;
    QSharedPointer<QStandardItemModel> myModel;
    QSharedPointer<QStandardItem> myRoot, myEmptyTemplate;
    QSharedPointer<QMenu> myMenu;
    QSharedPointer<QMenu> myInsertMenu;
    QAction * myDeleteAction;
  };
}

#endif // NOISEGRAPHBUILDER_H
