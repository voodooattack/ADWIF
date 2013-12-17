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
#include <QMimeData>

#include <QtGui/QWidget>
#include <QtGui/QTreeView>
#include <QStandardItemModel>
#include <QIdentityProxyModel>
#include <QMenu>

#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <QtTreePropertyBrowser>

#include <json/value.h>

#include "extendedvarianteditorfactory.hpp"
#include "extendedvariantmanager.hpp"

#include <iostream>

namespace ADWIF
{
  struct ModuleTemplate
  {
    QString name;
    QString jsonName;
    int sources;
    QString icon;
    QList<QtVariantProperty*> (*addToManager) (QtVariantPropertyManager&);
    Json::Value (*toJson) (QtVariantPropertyManager&);
    void (*fromJson) (QtVariantPropertyManager&, const Json::Value &);
  };
}

Q_DECLARE_METATYPE(ADWIF::ModuleTemplate)

namespace ADWIF
{
  extern const std::vector<ModuleTemplate> moduleTemplates;

  class NoiseModuleItem: public QObject, public QStandardItem
  {
    Q_OBJECT
  public:
    static const Qt::ItemDataRole IsEmptyRole = (Qt::ItemDataRole)((int)Qt::UserRole + 1);
    static const Qt::ItemDataRole ModuleTemplateRole = (Qt::ItemDataRole)((int)Qt::UserRole + 2);

    using QStandardItem::parent;

    NoiseModuleItem(): QStandardItem() { setupPropertyManager(); }
    NoiseModuleItem(const NoiseModuleItem & other): QStandardItem(other), myTemplate(other.myTemplate) {
      setupPropertyManager();
      setModuleTemplate(other.myTemplate);
    }
    NoiseModuleItem(const QString & text): QStandardItem(text) { setupPropertyManager(); }
    NoiseModuleItem(const QIcon & icon, const QString & text): QStandardItem(icon, text) { setupPropertyManager(); }

    virtual ~NoiseModuleItem() { for(auto i : myProperties) delete i; }

    void setupPropertyManager();

    virtual QStandardItem * clone() const
    {
      NoiseModuleItem * newItem = new NoiseModuleItem(*this);
      return newItem;
    }

    NoiseModuleItem & operator=(const NoiseModuleItem& other)
    {
      QStandardItem::operator=(other);
      setupPropertyManager();
      myTemplate = other.myTemplate;
      setModuleTemplate(other.myTemplate);
      return *this;
    }

    const ModuleTemplate & moduleTemplate() const { return myTemplate; }
    void setModuleTemplate(const ModuleTemplate & templ);

    const QString & name() const { return myTemplate.name; }
    void setName(const QString & name) { myTemplate.name = name; }

    void addToPropertyBrowser(QtTreePropertyBrowser * browser);

    Json::Value toJson() const;

    void fromJson(const Json::Value & value);

  public slots:
    void valueChanged(QtProperty *, const QVariant &) { }
  private:
    ModuleTemplate myTemplate;
    QSharedPointer<QtVariantPropertyManager> myManager;
    QSharedPointer<QtVariantEditorFactory> myFactory;
    QList<QtVariantProperty *> myProperties;
  };

  class NoiseGraphItemModel: public QIdentityProxyModel
  {
    Q_OBJECT
  public:
//     virtual Qt::DropActions supportedDragActions() const {
//       return Qt::DropAction::CopyAction;
//     }

    explicit NoiseGraphItemModel(QObject * parent = 0): QIdentityProxyModel(parent)
    {
    }

    virtual bool insertColumns(int column, int count, const QModelIndex& parent)
    {
      std::cerr << "alalalala\n";
      return QIdentityProxyModel::insertColumns(column, count, parent);
    }

    virtual bool insertRows(int row, int count, const QModelIndex& parent)
    {
      std::cerr << "trolololo\n";
      return QIdentityProxyModel::insertRows(row, count, parent);
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
    {
      if (row == -1 && column == -1)
      {
        QModelIndex target = mapToSource(parent);
        if (itemData(target)[NoiseModuleItem::IsEmptyRole].value<bool>())
        {
          removeRow(parent.row(), parent.parent());
          bool result = QIdentityProxyModel::dropMimeData(data, action, row, column, parent.parent());
          return result;
        }
        else
          return false;
      } else
        return false;
    }
//
//     Qt::ItemFlags flags(const QModelIndex &index) const
//     {
//       Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
//
//       if (index.isValid())
//         if (itemFromIndex(index)->isEditable() && itemFromIndex(index)->data().value<bool>())
//           return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
//         else
//           return defaultFlags & ~Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
//       else
//       {
//         return defaultFlags & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled;
//       }
//     }

  private:

  };

  class NoiseGraphBuilder: public QTreeView
  {
    Q_OBJECT
  public:
    explicit NoiseGraphBuilder(QWidget * parent = 0);

    void setPropertyBrowser(QtTreePropertyBrowser * propertyBrowser) { myPropertyBrowser = propertyBrowser; }

    bool isComplete() const;
    Json::Value toJson() const;

  private:
    bool isComplete(const QStandardItem * item) const;

    void dropEvent(QDropEvent * e);

  public slots:
    void onShowContextMenu(const QPoint & pt);
    void onActionTriggered();
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  private:
    QtTreePropertyBrowser * myPropertyBrowser;
    QSharedPointer<QStandardItem> myEmptyTemplate;
    QSharedPointer<QStandardItemModel> myModel;
    QSharedPointer<QAbstractProxyModel> myProxyModel;
    QSharedPointer<QMenu> myMenu;
    QSharedPointer<QMenu> myInsertMenu;
    QAction * myDeleteAction;
  };
}

#endif // NOISEGRAPHBUILDER_H
