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

#include "noisegraphbuilder.hpp"

#include <vector>

#include <QMenu>

namespace ADWIF
{
  const std::vector<ModuleTemplate> moduleTemplates =
  {
    { "Add", "Add", 2, ":/icons/resources/plus.png" },
    { "Multiply", "Multiply", 2, ":/icons/resources/asterisk.png" },
    { "Power", "Power", 2, ":/icons/resources/edit-superscript.png" },
    { "Min", "Min", 2, ":/icons/resources/calculator--minus.png" },
    { "Max", "Max", 2, ":/icons/resources/calculator--plus.png" },
    { },
    { "Abs", "Abs", 1, ":/icons/resources/calculator--arrow.png" },
    { "Invert", "Invert", 1, "" },
    { "Clamp", "Clamp", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Min")
                   << manager.addProperty(QVariant::Double, "Max");
        return properties;
      }
    },
    { },
    { "Select", "Select", 3, ":/icons/resources/node-select.png",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Falloff")
                   << manager.addProperty(QVariant::Double, "Lower Bound")
                   << manager.addProperty(QVariant::Double, "Upper Bound");
        return properties;
      }
    },
    { "Blend", "Blend", 3, ":/icons/resources/node.png" },
    { "Displace", "Displace", 4, "" },
    { "Curve", "Curve", 1, ":/icons/resources/calculator--arrow.png",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(ExtendedVariantManager::curve2dTypeId(), "Curve");
        return properties;
      }
    }
//     { "Terrace", "Terrace", 1, ":/icons/resources/calculator--arrow.png" },
  };

  NoiseGraphBuilder::NoiseGraphBuilder(QWidget * parent): QTreeView(parent)
  {
    setContextMenuPolicy(Qt::CustomContextMenu);

    setAlternatingRowColors(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setSelectionMode(SelectionMode::SingleSelection);
    setSelectionBehavior(SelectionBehavior::SelectRows);

    QObject::connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onShowContextMenu(QPoint)));

    myModel = QSharedPointer<QStandardItemModel>(new QStandardItemModel);
    myRoot = QSharedPointer<QStandardItem>(new QStandardItem);
    myEmptyTemplate = QSharedPointer<NoiseModuleItem>(new NoiseModuleItem(QIcon(":/icons/resources/document-insert.png"), "Empty"));
    myEmptyTemplate->setEditable(false);

    myRoot->setText("Graph");
    myRoot->setIcon(QIcon(":/icons/resources/document-tree.png"));
    myRoot->setEditable(false);

    myRoot->appendRow(myEmptyTemplate->clone());
    myRoot->setBackground(Qt::red);

    myModel->invisibleRootItem()->appendRow(myRoot.data());
    setModel(myModel.data());

    myMenu = QSharedPointer<QMenu>(new QMenu);
    myInsertMenu = QSharedPointer<QMenu>(new QMenu);
    myInsertMenu->setIcon(QIcon(":/icons/resources/document-hf-insert.png"));
    myInsertMenu->setTitle("&Insert");

    for (const ModuleTemplate & t: moduleTemplates)
    {
      if (!t.name.isEmpty())
      {
        QAction * action = myInsertMenu->addAction(QIcon(t.icon.c_str()), t.name);
        action->setData(QVariant::fromValue((void*)&t));
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(onActionTriggered()));
      } else
        myInsertMenu->addSeparator();
    }

    myMenu->addMenu(myInsertMenu.data());
    myDeleteAction = myMenu->addAction(QIcon(":/icons/resources/document-hf-delete.png"), "&Delete");

    expandAll();
  }

  void NoiseGraphBuilder::onShowContextMenu(const QPoint & pt)
  {
    if (selectedIndexes().empty())
      return;

    if (selectedIndexes().size() == 1)
    {
      if (myModel->itemFromIndex(selectedIndexes()[0])->isEditable() ||
          myModel->itemFromIndex(selectedIndexes()[0]) == myRoot.data())
        myInsertMenu->setEnabled(false);
      else
        myInsertMenu->setEnabled(true);
    }
    else
      myInsertMenu->setEnabled(false);

    if (!myModel->itemFromIndex(selectedIndexes()[0])->isEditable() ||
        !myModel->itemFromIndex(selectedIndexes()[0])->data().value<void*>())
      myDeleteAction->setEnabled(false);
    else
      myDeleteAction->setEnabled(true);

    myMenu->popup(mapToGlobal(pt));
  }

  void NoiseGraphBuilder::onActionTriggered()
  {
    QAction * action = dynamic_cast<QAction*>(QObject::sender());

    if (action == myDeleteAction)
    {
      myModel->removeRow(selectedIndexes()[0].row());
    }
    else
    {
      const ModuleTemplate * templ = reinterpret_cast<const ModuleTemplate *>(action->data().value<void*>());
      NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(selectedIndexes()[0]));
      item->setIcon(QIcon(templ->icon.c_str()));
      item->setText(templ->name);
      item->setEditable(true);
      item->setData(QVariant::fromValue((void*)templ));
      item->setModuleTemplate(*templ);
      if (!item->parent()->data().value<void*>())
        item->parent()->setBackground(Qt::green);
      else
      {
        item->parent()->setBackground(Qt::green);
        for (int i = 0; i < item->parent()->rowCount(); i++)
        {
          if (item->parent()->child(i)->isEditable() == false)
          {
            item->parent()->setBackground(Qt::red);
            break;
          }
        }
      }
      for (int i = 0; i < templ->sources; i++)
        item->appendRow(myEmptyTemplate->clone());
      if (item->rowCount())
        item->setBackground(Qt::red);
      else
        item->setBackground(Qt::green);
      expand(selectedIndexes()[0]);
    }
  }

  void NoiseGraphBuilder::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
  {
    QTreeView::selectionChanged(selected, deselected);
    NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(selected.indexes()[0]));
    myPropertyBrowser->clear();
    if (item)
      item->addToPropertyBrowser(myPropertyBrowser);
    myPropertyBrowser->update();
  }

  bool NoiseGraphBuilder::isComplete() const
  {
    return isComplete(myRoot->child(0));
  }

  bool NoiseGraphBuilder::isComplete(const QStandardItem * item) const
  {
    if (!item || !item->data().value<void*>())
      return false;
    for (int i = 0; i < item->rowCount(); i++)
      if (!item->child(i)->data().value<void*>() || !isComplete(item->child(i)))
        return false;
    return true;
  }
}
