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
#include <iostream>

#include <QMenu>

namespace ADWIF
{
  QtVariantProperty * property(QtVariantPropertyManager & manager, const QString & name)
  {
    for(QtProperty * p : manager.properties())
      if (p->propertyName() == name) return manager.variantProperty(p);
    return nullptr;
  }

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
    { "Cache", "Cache", 1, "" },
    { "Clamp", "Clamp", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Min")
                   << manager.addProperty(QVariant::Double, "Max");
        properties[0]->setPropertyId("min");
        properties[1]->setPropertyId("max");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["min"] = property(manager, "Min")->value().value<double>();
        val["max"] = property(manager, "Max")->value().value<double>();
        return val;
      }
    },
    { },
    { "Scale/Bias", "ScaleBias", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Scale")
                   << manager.addProperty(QVariant::Double, "Bias");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["scale"] = property(manager, "Scale")->value().value<double>();
        val["bias"] = property(manager, "Bias")->value().value<double>();
        return val;
      }
    },
    { "Scale Point", "ScalePoint", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Vector3D, "Scale");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        QVector3D vec = property(manager, "Scale")->value().value<QVector3D>();
        val["scale"][0] = vec.x();
        val["scale"][1] = vec.y();
        val["scale"][2] = vec.z();
        return val;
      }
    },
    { "Translate", "Translate", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Vector3D, "Translation");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        QVector3D vec = property(manager, "Translation")->value().value<QVector3D>();
        val["translation"][0] = vec.x();
        val["translation"][1] = vec.y();
        val["translation"][2] = vec.z();
        return val;
      }
    },
    { "Rotate", "Rotate", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Vector3D, "Rotation");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        QVector3D vec = property(manager, "Rotation")->value().value<QVector3D>();
        val["rotation"][0] = vec.x();
        val["rotation"][1] = vec.y();
        val["rotation"][2] = vec.z();
        return val;
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
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["falloff"] = property(manager, "Falloff")->value().value<double>();
        val["bounds"][0] = property(manager, "Lower Bound")->value().value<double>();
        val["bounds"][1] = property(manager, "Upper Bound")->value().value<double>();
        return val;
      }
    },
    { "Blend", "Blend", 3, ":/icons/resources/node.png" },
    { "Displace", "Displace", 4, "" },
    { "Curve", "Curve", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(ExtendedVariantManager::curve2dTypeId(), "Curve");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["curve"] = Json::Value::null;
        Curve2D curve = property(manager, "Curve")->value().value<Curve2D>();
        for (const auto & p : curve)
        {
          Json::Value v = Json::Value::null;
          v[0] = p.x();
          v[1] = p.y();
          val["curve"].append(v);
        }
        return val;
      }
    },
    { "Terrace", "Terrace", 1, "", // TODO: Terrace curve for editor
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(ExtendedVariantManager::curve2dTypeId(), "Curve");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["curve"] = Json::Value::null;
        Curve2D curve = property(manager, "Curve")->value().value<Curve2D>();
        for (const auto & p : curve)
        {
          val["curve"].append(p.x());
        }
        return val;
      }
    },
    { "Turbulence", "Turbulence", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Power")
                   << manager.addProperty(QVariant::Double, "Roughness")
                   << manager.addProperty(QVariant::Int, "Seed");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["power"] = property(manager, "Power")->value().value<double>();
        val["roughness"] = property(manager, "Roughness")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        return val;
      }
    },
    { },
    { "Constant", "Constant", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Value");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["value"] = property(manager, "Value")->value().value<double>();
        return val;
      }
    },
    { "Checkerboard", "Checkerboard", 0, "" },
    { "Billow", "Billow", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Lacunarity")
                   << manager.addProperty(QVariant::Double, "Persistence")
                   << manager.addProperty(QVariant::Int, "Seed")
                   << manager.addProperty(QVariant::UInt, "Octaves");
        QtVariantProperty * quality = manager.addProperty(manager.enumTypeId(), "Quality");
        QStringList qualityNames;
        qualityNames << "Fast" << "Standard" << "Best";
        quality->setAttribute("enumNames", qualityNames);
        properties << quality;
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["lacunarity"] = property(manager, "Lacunarity")->value().value<double>();
        val["persistence"] = property(manager, "Persistence")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        val["octaves"] = property(manager, "Octaves")->value().value<uint>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      }
    },
    { "Perlin Noise", "Perlin", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Lacunarity")
                   << manager.addProperty(QVariant::Double, "Persistence")
                   << manager.addProperty(QVariant::Int, "Seed")
                   << manager.addProperty(QVariant::UInt, "Octaves");
        QtVariantProperty * quality = manager.addProperty(manager.enumTypeId(), "Quality");
        QStringList qualityNames;
        qualityNames << "fast" << "standard" << "best";
        quality->setAttribute("enumNames", qualityNames);
        properties << quality;
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["lacunarity"] = property(manager, "Lacunarity")->value().value<double>();
        val["persistence"] = property(manager, "Persistence")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        val["octaves"] = property(manager, "Octaves")->value().value<uint>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      }
    },
    { "Ridged Noise", "RidgedMulti", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Lacunarity")
                   << manager.addProperty(QVariant::Int, "Seed")
                   << manager.addProperty(QVariant::UInt, "Octaves");
        QtVariantProperty * quality = manager.addProperty(manager.enumTypeId(), "Quality");
        QStringList qualityNames;
        qualityNames << "fast" << "standard" << "best";
        quality->setAttribute("enumNames", qualityNames);
        properties << quality;
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["lacunarity"] = property(manager, "Lacunarity")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        val["octaves"] = property(manager, "Octaves")->value().value<uint>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      }
    },
    { "Voronoi Diagram", "Voronoi", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Displacement")
                   << manager.addProperty(QVariant::Int, "Seed");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["displacement"] = property(manager, "Displacement")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        return val;
      }
    },
    { "Cylinders", "Cylinders", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        return val;
      }
    },
    { "Spheres", "Spheres", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency");
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        return val;
      }
    },
    { "Heightmap Source", "Heightmap", 0, "" }
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
    QObject::connect(myDeleteAction, SIGNAL(triggered()), this, SLOT(onActionTriggered()));

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
      NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(selectedIndexes()[0]));
      item->setModuleTemplate(ModuleTemplate());
      item->setText("Empty");
      item->setIcon(QIcon(":/icons/resources/document-insert.png"));
      item->setEditable(false);
      item->removeRows(0, item->rowCount());
      item->setBackground(myEmptyTemplate->background());
      item->setData(QVariant::fromValue((void*)0));
      item->parent()->setBackground(Qt::red);
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
        item->parent()->setBackground(myEmptyTemplate->background());
      else
      {
        item->parent()->setBackground(myEmptyTemplate->background());
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
        item->setBackground(myEmptyTemplate->background());
      myPropertyBrowser->clear();
      item->addToPropertyBrowser(myPropertyBrowser);
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

  Json::Value NoiseGraphBuilder::toJson() const
  {
    if (!isComplete())
      return Json::Value();
    if (auto item = dynamic_cast<NoiseModuleItem*>(myRoot->child(0)))
      return item->toJson();
    else
      return Json::Value();
  }
}
