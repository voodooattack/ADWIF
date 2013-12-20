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
#include <QDropEvent>

#include "noiseutils.hpp"

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
    { },
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
        property(manager, "Min")->setAttribute("decimals", 4);
        property(manager, "Min")->setAttribute("singleStep", 0.01);
        property(manager, "Max")->setAttribute("decimals", 4);
        property(manager, "Max")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["min"] = property(manager, "Min")->value().value<double>();
        val["max"] = property(manager, "Max")->value().value<double>();
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["min"].isDouble())
          property(manager, "Min")->setValue(val["min"].asDouble());
        if (val["max"].isDouble())
          property(manager, "Max")->setValue(val["max"].asDouble());
      }
    },
    { },
    { "Scale/Bias", "ScaleBias", 1, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Scale")
                   << manager.addProperty(QVariant::Double, "Bias");
        property(manager, "Scale")->setAttribute("decimals", 4);
        property(manager, "Scale")->setAttribute("singleStep", 0.01);
        property(manager, "Bias")->setAttribute("decimals", 4);
        property(manager, "Bias")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["scale"] = property(manager, "Scale")->value().value<double>();
        val["bias"] = property(manager, "Bias")->value().value<double>();
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["scale"].isDouble())
          property(manager, "Scale")->setValue(val["scale"].asDouble());
        if (val["bias"].isDouble())
          property(manager, "Bias")->setValue(val["bias"].asDouble());
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["scale"].isArray())
        {
          QVector3D vec;
          vec.setX(val["scale"][0].asDouble());
          vec.setY(val["scale"][1].asDouble());
          vec.setZ(val["scale"][2].asDouble());
          property(manager, "Scale")->setValue(vec);
        }
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["translation"].isArray())
        {
          QVector3D vec;
          vec.setX(val["translation"][0].asDouble());
          vec.setY(val["translation"][1].asDouble());
          vec.setZ(val["translation"][2].asDouble());
          property(manager, "Translation")->setValue(vec);
        }
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["rotation"].isArray())
        {
          QVector3D vec;
          vec.setX(val["rotation"][0].asDouble());
          vec.setY(val["rotation"][1].asDouble());
          vec.setZ(val["rotation"][2].asDouble());
          property(manager, "Rotation")->setValue(vec);
        }
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
        property(manager, "Falloff")->setAttribute("decimals", 4);
        property(manager, "Falloff")->setAttribute("singleStep", 0.01);
        property(manager, "Lower Bound")->setAttribute("decimals", 4);
        property(manager, "Lower Bound")->setAttribute("singleStep", 0.01);
        property(manager, "Upper Bound")->setAttribute("decimals", 4);
        property(manager, "Upper Bound")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["falloff"] = property(manager, "Falloff")->value().value<double>();
        val["bounds"][0] = property(manager, "Lower Bound")->value().value<double>();
        val["bounds"][1] = property(manager, "Upper Bound")->value().value<double>();
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["falloff"].isDouble())
          property(manager, "Falloff")->setValue(val["falloff"].asDouble());
        if (val["bounds"].isArray())
        {
          property(manager, "Lower Bound")->setValue(val["bounds"][0].asDouble());
          property(manager, "Upper Bound")->setValue(val["bounds"][1].asDouble());
        }
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["curve"].isArray())
        {
          QPolygonF poly;
          for (const Json::Value & v : val["curve"])
            poly << QPointF(v[0].asDouble(), v[1].asDouble());
          property(manager, "Curve")->setValue(QVariant::fromValue(poly));
        }
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["curve"].isArray())
        {
          QPolygonF poly;
          for (const Json::Value & v : val["curve"])
            poly << QPointF(v.asDouble(), 0);
          property(manager, "Curve")->setValue(QVariant::fromValue(poly));
        }
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
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        property(manager, "Power")->setAttribute("decimals", 4);
        property(manager, "Power")->setAttribute("singleStep", 0.01);
        property(manager, "Roughness")->setAttribute("decimals", 4);
        property(manager, "Roughness")->setAttribute("singleStep", 0.01);
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
        if (val["power"].isDouble())
          property(manager, "Power")->setValue(val["power"].asDouble());
        if (val["roughness"].isDouble())
          property(manager, "Roughness")->setValue(val["roughness"].asDouble());
        if (val["seed"].isInt())
          property(manager, "Seed")->setValue(val["seed"].asDouble());
      }
    },
    { },
    { "Constant", "Constant", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Value");
        property(manager, "Value")->setAttribute("decimals", 4);
        property(manager, "Value")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["value"] = property(manager, "Value")->value().value<double>();
        return val;
      }
      ,
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["value"].isDouble())
          property(manager, "Value")->setValue(val["value"].asDouble());
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
                   << manager.addProperty(QVariant::Int, "Octaves");
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        property(manager, "Lacunarity")->setAttribute("decimals", 4);
        property(manager, "Lacunarity")->setAttribute("singleStep", 0.01);
        property(manager, "Persistence")->setAttribute("decimals", 4);
        property(manager, "Persistence")->setAttribute("singleStep", 0.01);
        property(manager, "Octaves")->setAttribute("minimum", 1);
        property(manager, "Octaves")->setAttribute("maximum", noise::module::BILLOW_MAX_OCTAVE);
        property(manager, "Octaves")->setValue(1);
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
        val["octaves"] = property(manager, "Octaves")->value().value<int>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
        if (val["lacunarity"].isDouble())
          property(manager, "Lacunarity")->setValue(val["lacunarity"].asDouble());
        if (val["persistence"].isDouble())
          property(manager, "Persistence")->setValue(val["persistence"].asDouble());
        if (val["seed"].isInt())
          property(manager, "Seed")->setValue(val["seed"].asDouble());
        if (val["octaves"].isUInt())
          property(manager, "Octaves")->setValue(val["octaves"].asUInt());
        if (val["quality"].isString())
        {
          if (val["quality"].asString() == "fast")
            property(manager, "Quality")->setValue(0);
          else if (val["quality"].asString() == "standard")
            property(manager, "Quality")->setValue(1);
          else if (val["quality"].asString() == "best")
            property(manager, "Quality")->setValue(2);
        }
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
                   << manager.addProperty(QVariant::Int, "Octaves");
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        property(manager, "Lacunarity")->setAttribute("decimals", 4);
        property(manager, "Lacunarity")->setAttribute("singleStep", 0.01);
        property(manager, "Persistence")->setAttribute("decimals", 4);
        property(manager, "Persistence")->setAttribute("singleStep", 0.01);
        property(manager, "Octaves")->setAttribute("minimum", 1);
        property(manager, "Octaves")->setAttribute("maximum", noise::module::PERLIN_MAX_OCTAVE);
        property(manager, "Octaves")->setValue(1);
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
        val["octaves"] = property(manager, "Octaves")->value().value<int>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
        if (val["lacunarity"].isDouble())
          property(manager, "Lacunarity")->setValue(val["lacunarity"].asDouble());
        if (val["persistence"].isDouble())
          property(manager, "Persistence")->setValue(val["persistence"].asDouble());
        if (val["seed"].isInt())
          property(manager, "Seed")->setValue(val["seed"].asDouble());
        if (val["octaves"].isUInt())
          property(manager, "Octaves")->setValue(val["octaves"].asUInt());
        if (val["quality"].isString())
        {
          if (val["quality"].asString() == "fast")
            property(manager, "Quality")->setValue(0);
          else if (val["quality"].asString() == "standard")
            property(manager, "Quality")->setValue(1);
          else if (val["quality"].asString() == "best")
            property(manager, "Quality")->setValue(2);
        }
      }
    },
    { "Ridged Noise", "RidgedMulti", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Lacunarity")
                   << manager.addProperty(QVariant::Int, "Seed")
                   << manager.addProperty(QVariant::Int, "Octaves");
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        property(manager, "Lacunarity")->setAttribute("decimals", 4);
        property(manager, "Lacunarity")->setAttribute("singleStep", 0.01);
        property(manager, "Octaves")->setAttribute("minimum", 1);
        property(manager, "Octaves")->setAttribute("maximum", noise::module::RIDGED_MAX_OCTAVE);
        property(manager, "Octaves")->setValue(1);
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
        val["octaves"] = property(manager, "Octaves")->value().value<int>();
        switch(property(manager, "Quality")->value().value<int>())
        {
          case 0: val["quality"] = "fast"; break;
          case 1: val["quality"] = "standard"; break;
          case 2: val["quality"] = "best"; break;
        }
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
        if (val["lacunarity"].isDouble())
          property(manager, "Lacunarity")->setValue(val["lacunarity"].asDouble());
        if (val["seed"].isInt())
          property(manager, "Seed")->setValue(val["seed"].asDouble());
        if (val["octaves"].isUInt())
          property(manager, "Octaves")->setValue(val["octaves"].asUInt());
        if (val["quality"].isString())
        {
          if (val["quality"].asString() == "fast")
            property(manager, "Quality")->setValue(0);
          else if (val["quality"].asString() == "standard")
            property(manager, "Quality")->setValue(1);
          else if (val["quality"].asString() == "best")
            property(manager, "Quality")->setValue(2);
        }
      }
    },
    { "Voronoi Diagram", "Voronoi", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency")
                   << manager.addProperty(QVariant::Double, "Displacement")
                   << manager.addProperty(QVariant::Int, "Seed");
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        property(manager, "Displacement")->setAttribute("decimals", 4);
        property(manager, "Displacement")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        val["displacement"] = property(manager, "Displacement")->value().value<double>();
        val["seed"] = property(manager, "Seed")->value().value<int>();
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
        if (val["displacement"].isDouble())
          property(manager, "Displacement")->setValue(val["displacement"].asDouble());
        if (val["seed"].isInt())
          property(manager, "Seed")->setValue(val["seed"].asDouble());
      }
    },
    { "Cylinders", "Cylinders", 0, "",
      [](QtVariantPropertyManager & manager) -> QList<QtVariantProperty*>
      {
        QList<QtVariantProperty*> properties;
        properties << manager.addProperty(QVariant::Double, "Frequency");
        property(manager, "Frequency")->setAttribute("decimals", 4);
        property(manager, "Frequency")->setAttribute("singleStep", 0.01);
        return properties;
      },
      [](QtVariantPropertyManager & manager) -> Json::Value
      {
        Json::Value val = Json::Value::null;
        val["frequency"] = property(manager, "Frequency")->value().value<double>();
        return val;
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
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
      },
      [](QtVariantPropertyManager & manager, const Json::Value & val)
      {
        if (val["frequency"].isDouble())
          property(manager, "Frequency")->setValue(val["frequency"].asDouble());
      }
    },
    { "Heightmap Source", "Heightmap", 0, "" }
  };

  NoiseGraphBuilder::NoiseGraphBuilder(QWidget * parent): QTreeView(parent)
  {
    qRegisterMetaType<ModuleTemplate>("ModuleTemplate");

    setContextMenuPolicy(Qt::CustomContextMenu);

    setAlternatingRowColors(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setSelectionMode(SelectionMode::SingleSelection);
    setSelectionBehavior(SelectionBehavior::SelectRows);

    QObject::connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onShowContextMenu(QPoint)));

    myModel = QSharedPointer<QStandardItemModel>(new QStandardItemModel(this));
    myProxyModel = QSharedPointer<QAbstractProxyModel>(new NoiseGraphItemModel(this));
    myProxyModel->setSourceModel(myModel.data());

    setModel(myProxyModel.data());

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(DragDropMode::DragDrop);
    setDefaultDropAction(Qt::DropAction::MoveAction);
    setDragDropOverwriteMode(true);

    myEmptyTemplate = QSharedPointer<NoiseModuleItem>(
      new NoiseModuleItem(QIcon(":/icons/resources/document-insert.png"), "Empty"));
    myEmptyTemplate->setEditable(false);
    myEmptyTemplate->setData(true, NoiseModuleItem::IsEmptyRole);
    myModel->setItemPrototype(myEmptyTemplate->clone());
    myModel->invisibleRootItem()->appendRow(myEmptyTemplate->clone());

    myMenu = QSharedPointer<QMenu>(new QMenu);
    myInsertMenu = QSharedPointer<QMenu>(new QMenu);
    myInsertMenu->setIcon(QIcon(":/icons/resources/document-hf-insert.png"));
    myInsertMenu->setTitle("&Insert");

    for (int i = 1; i < moduleTemplates.size(); i++)
    {
      if (!moduleTemplates[i].name.isEmpty())
      {
        QAction * action = myInsertMenu->addAction(QIcon(moduleTemplates[i].icon), moduleTemplates[i].name);
        action->setData(i);
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(onActionTriggered()));
      } else
        myInsertMenu->addSeparator();
    }

    myMenu->addMenu(myInsertMenu.data());
    myDeleteAction = myMenu->addAction(QIcon(":/icons/resources/document-hf-delete.png"), "&Delete");
    QObject::connect(myDeleteAction, SIGNAL(triggered()), this, SLOT(onActionTriggered()));

    expandAll();
  }

  void NoiseGraphBuilder::dropEvent(QDropEvent * e)
  {
    QModelIndex isrc = myProxyModel->mapToSource(currentIndex());
    QModelIndex idst = myProxyModel->mapToSource(indexAt(e->pos()));

    QStandardItem * src = myModel->itemFromIndex(isrc);
    QStandardItem * dst = myModel->itemFromIndex(idst);
    QStandardItem * psrc = myModel->itemFromIndex(isrc.parent());
    QStandardItem * pdst = myModel->itemFromIndex(idst.parent());

    if (!src || !dst)
    {
      e->ignore();
      return;
    }

    if (src->data(NoiseModuleItem::IsEmptyRole).value<bool>())
    {
      e->ignore();
      return;
    }

    if (isDescendantOf(isrc, idst) || isDescendantOf(idst, isrc))
    {
      e->ignore();
      return;
    }

    if (e->proposedAction() == Qt::DropAction::MoveAction)
    {
      clearSelection();

      src = psrc->takeChild(isrc.row());
      dst = pdst->takeChild(idst.row());

      psrc->setChild(isrc.row(), dst);
      pdst->setChild(idst.row(), src);

      e->acceptProposedAction();

      myProxyModel->revert();
    }
    else if (e->proposedAction() == Qt::DropAction::CopyAction)
    {
      clearSelection();

      src = recursiveClone(src);
      pdst->setChild(idst.row(), src);

      e->acceptProposedAction();

      myProxyModel->revert();
    }
    else
      e->ignore();
  }

  void NoiseGraphBuilder::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
  {
    QTreeView::selectionChanged(selected, deselected);

    if (selected.empty()) return;

    QModelIndex idx = myProxyModel->mapToSource(currentIndex());
    NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(idx));
    myPropertyBrowser->clear();
    if (item)
      item->addToPropertyBrowser(myPropertyBrowser);
    myPropertyBrowser->update();
  }

  void NoiseGraphBuilder::onShowContextMenu(const QPoint & pt)
  {
    if (selectedIndexes().empty())
      return;

    if (selectedIndexes().size() == 1)
    {
      QModelIndex idx = myProxyModel->mapToSource(currentIndex());
      if ( myModel->itemFromIndex(idx)->isEditable() ||
          !myModel->itemFromIndex(idx)->data(NoiseModuleItem::IsEmptyRole).value<bool>())
        myInsertMenu->setEnabled(false);
      else
        myInsertMenu->setEnabled(true);

      if (!myModel->itemFromIndex(idx)->isEditable() ||
           myModel->itemFromIndex(idx)->data(NoiseModuleItem::IsEmptyRole).value<bool>())
        myDeleteAction->setEnabled(false);
      else
        myDeleteAction->setEnabled(true);
    }
    else
    {
      myDeleteAction->setEnabled(false);
      myInsertMenu->setEnabled(false);
    }

    myMenu->popup(mapToGlobal(pt));
  }

  void NoiseGraphBuilder::onActionTriggered()
  {
    QAction * action = dynamic_cast<QAction*>(QObject::sender());

    QModelIndex idx = myProxyModel->mapToSource(currentIndex());

    if (action == myDeleteAction)
    {
      NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(idx));
      item->setModuleTemplate(ModuleTemplate());
      item->setText("Empty");
      item->setIcon(QIcon(":/icons/resources/document-insert.png"));
      item->setEditable(false);
      item->removeRows(0, item->rowCount());
      item->setBackground(myModel->invisibleRootItem()->background());
      item->setData(true, NoiseModuleItem::IsEmptyRole);
      if (item->parent())
        item->parent()->setBackground(Qt::red);
    }
    else
    {
      const ModuleTemplate & templ = moduleTemplates[action->data().value<int>()];

      NoiseModuleItem * item = dynamic_cast<NoiseModuleItem*>(myModel->itemFromIndex(idx));
      item->setModuleTemplate(templ);

      if (item->parent())
      {
        item->parent()->setBackground(myModel->invisibleRootItem()->background());
        for (int i = 0; i < item->parent()->rowCount(); i++)
        {
          if (item->parent()->child(i)->data(NoiseModuleItem::IsEmptyRole).value<bool>() == true)
          {
            item->parent()->setBackground(Qt::red);
            break;
          }
        }
      }

      for (int i = 0; i < templ.sources; i++)
      {
        QStandardItem * clone = myEmptyTemplate->clone();
        clone->setData(true, NoiseModuleItem::IsEmptyRole);
        item->appendRow(clone);
      }

      if (item->rowCount())
        item->setBackground(Qt::red);
      else
        item->setBackground(myModel->invisibleRootItem()->background());
      myPropertyBrowser->clear();
      item->addToPropertyBrowser(myPropertyBrowser);
      expand(currentIndex());
    }
  }

  bool NoiseGraphBuilder::isComplete() const
  {
    return isComplete(myModel->invisibleRootItem()->child(0));
  }

  bool NoiseGraphBuilder::isComplete(const QStandardItem * item) const
  {
    if (!item || item->data(NoiseModuleItem::IsEmptyRole).value<bool>())
      return false;
    for (int i = 0; i < item->rowCount(); i++)
      if (item->child(i)->data(NoiseModuleItem::IsEmptyRole).value<bool>() || !isComplete(item->child(i)))
        return false;
    return true;
  }

  void NoiseGraphBuilder::clear()
  {
    myModel->clear();
    myProxyModel->revert();
    myModel->invisibleRootItem()->appendRow(myEmptyTemplate->clone());
  }

  Json::Value NoiseGraphBuilder::toJson() const
  {
    if (!isComplete())
      return Json::Value();
    if (auto item = dynamic_cast<NoiseModuleItem*>(myModel->invisibleRootItem()->child(0)))
      return item->toJson();
    else
      return Json::Value();
  }

  bool NoiseGraphBuilder::fromJson(const Json::Value & val)
  {
    if (auto item = dynamic_cast<NoiseModuleItem*>(myModel->invisibleRootItem()->child(0)))
    {
      clear();
      item->fromJson(val);
      return true;
    } else
      return false;
  }

  QStandardItem * NoiseGraphBuilder::recursiveClone(QStandardItem * src)
  {
    QStandardItem * clone = src->clone();
    for (int i = 0; i < src->rowCount(); i++)
    {
      QList<QStandardItem *> row;
      for (int j = 0; j < src->columnCount(); j++)
        row.push_back(recursiveClone(src->child(i, j)));
      clone->insertRow(i, row);
    }
    return clone;
  }

  bool NoiseGraphBuilder::isDescendantOf(const QModelIndex & index, const QModelIndex & parent)
  {
    if (!parent.isValid())
      return false;
    else if (index.parent() == parent)
      return true;
    else
      return isDescendantOf(index, index.parent());
  }

  void NoiseModuleItem::setupPropertyManager()
  {
    myProperties.clear();
    myManager = QSharedPointer<ExtendedVariantManager>(new ExtendedVariantManager);
    myFactory = QSharedPointer<ExtendedVariantEditorFactory>(new ExtendedVariantEditorFactory);
    myFactory->addPropertyManager(myManager.data());
    QObject::connect(myManager.data(), SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                     this, SLOT(valueChanged(QtProperty *, const QVariant &)));
  }

  void NoiseModuleItem::setModuleTemplate(const ModuleTemplate & templ)
  {
    myTemplate = templ;
    myProperties.clear();
    myManager->clear();
    setData(QVariant::fromValue(templ), ModuleTemplateRole);
    if (!templ.jsonName.isEmpty())
    {
      if (templ.addToManager)
        myProperties = templ.addToManager(*myManager);
      setText(templ.name);
      setIcon(QIcon(templ.icon));
      setEditable(true);
      setData(false, IsEmptyRole);
    }
  }

  void NoiseModuleItem::addToPropertyBrowser(QtTreePropertyBrowser * browser)
  {
    for (int i = 0; i < myProperties.size(); i++)
      browser->addProperty(myProperties[i]);
    browser->setFactoryForManager(myManager.data(), myFactory.data());
  }

  Json::Value NoiseModuleItem::toJson() const
  {
    Json::Value value = Json::Value::null;
    ModuleTemplate templ = data(ModuleTemplateRole).value<ModuleTemplate>();
    if (templ.toJson) value = templ.toJson(*myManager);
    value["module"] = templ.jsonName.toStdString();
    if (text() != templ.name)
      value["name"] = text().toStdString();
    if (rowCount())
      value["sources"] = Json::Value::null;
    for (int i = 0; i < rowCount(); i++)
      if (auto m = dynamic_cast<NoiseModuleItem *>(child(i)))
        value["sources"][i] = m->toJson();
    return value;
  }

  void NoiseModuleItem::fromJson(const Json::Value & value)
  {
    if (myTemplate.fromJson) myTemplate.fromJson(*myManager, value);
    if (!value["sources"].empty())
    {
      for (int i = 0; i < value["sources"].size(); i++)
      {
        NoiseModuleItem * child = new NoiseModuleItem;
        QString moduleName = value["sources"][i]["module"].asCString();
        for (const ModuleTemplate & m : moduleTemplates)
        {
          QString jsonName = m.jsonName;
          if (!jsonName.compare(moduleName, Qt::CaseInsensitive))
          {
            child->setModuleTemplate(m);
            break;
          }
        }
        child->fromJson(value["sources"][i]);
        insertRow(rowCount(), child);
      }
    }
  }
}

