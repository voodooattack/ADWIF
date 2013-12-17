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

#include "editor.hpp"
#include "ui_editor.h"

#include "engine.hpp"
#include "game.hpp"
#include "mapgenerator.hpp"
#include "heightmapeditor.hpp"

#include <QtOpenGL/QGLWidget>
#include <QtCore/QTimer>
#include <QtGui/QGraphicsPolygonItem>

#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <QtTreePropertyBrowser>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon.hpp>
#include <boost/geometry/io/wkt/write.hpp>

#include <boost/filesystem/operations.hpp>

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(QPointF, double, cs::cartesian, x, y, setX, setY)
BOOST_GEOMETRY_REGISTER_RING(QPolygonF)

namespace ADWIF
{
  class RegionPolygonItem: public QGraphicsPolygonItem
  {
  public:
    RegionPolygonItem(Editor * editor, QGraphicsItem * parent = 0, QGraphicsScene * scene = 0) :
      QGraphicsPolygonItem(parent, scene), myEditor(editor)
      {
        QStringList biomes;
        for (const auto & b : myEditor->game()->biomes())
          biomes << b.second->name.c_str();
        pname = manager.addProperty(QVariant::String, "Name");
        pbiome = manager.addProperty(QtVariantPropertyManager::enumTypeId(), "Biome");
        pbiome->setAttribute("enumNames", biomes);
      }
  protected:
    virtual void focusInEvent(QFocusEvent * event)
    {
      QGraphicsPolygonItem::focusInEvent(event);
      std::string name = myEditor->game()->generator()->regions()[data(0).toInt()].name;
      std::string biome = myEditor->game()->generator()->regions()[data(0).toInt()].biome;
      if (name.empty()) name = "Region " + boost::lexical_cast<std::string>(data(0).toInt());
      std::string disp = name + " (" + biome + ")";

      myEditor->ui()->textCurrentSelection->setText(disp.c_str());
      myEditor->ui()->propertyBrowser->clear();

      int bidx = 0;
      int bcount = 0;
      for (const auto & b : myEditor->game()->biomes())
      {
        if (biome == b.second->name)
          bidx = bcount;
        bcount++;
      }

      pname->setValue(QString(name.c_str()));
      pbiome->setValue(bidx);

      myEditor->ui()->propertyBrowser->setFactoryForManager(&manager, &factory);
      myEditor->ui()->propertyBrowser->addProperty(pname);
      myEditor->ui()->propertyBrowser->addProperty(pbiome);

//       std::cerr << boost::geometry::wkt(polygon()) << std::endl << std::endl;
    }

    virtual void focusOutEvent(QFocusEvent * event)
    {
      QGraphicsPolygonItem::focusOutEvent(event);
    }

    Editor * myEditor;
    QtVariantPropertyManager manager;
    QtVariantEditorFactory factory;
    QtVariantProperty * pname, * pbiome;
  };

  Editor::Editor(const std::shared_ptr<Engine> & engine): myEngine(engine), myProgressTimer(nullptr)
  {
    myUi = myUi.create();
    myUi->setupUi(this);

    std::shared_ptr<EditorLogProvider> logProvider(new EditorLogProvider);
    myEngine->logProvider(logProvider);

    qRegisterMetaType<LogLevel>("LogLevel");

    QObject::connect(myUi->action_Create_Map, SIGNAL(triggered()), this, SLOT(createMap()));
    QObject::connect(myUi->action_Reload, SIGNAL(triggered()), this, SLOT(reloadData()));
    QObject::connect(myUi->action_Terrain_Parameters, SIGNAL(triggered()), this, SLOT(showHeightMapEditor()));
    QObject::connect(logProvider.get(),
                     SIGNAL(onMessage(LogLevel, const QString &, const QString &)),
                     this,
                     SLOT(onMessage(LogLevel, const QString &, const QString &)));
    QObject::connect(this, SIGNAL(onDataReloaded()), this, SLOT(dataChanged()));
    QObject::connect(this, SIGNAL(onMapReady()), this, SLOT(refreshMap()));

    myUi->tableLog->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myUi->tableLog->setSelectionBehavior(QAbstractItemView::SelectRows);
    myUi->tableLog->setSelectionMode(QAbstractItemView::SingleSelection);
    myUi->tableLog->verticalHeader()->setVisible(false);

    myStatusLabel = new QLabel("Ready");
    myUi->statusbar->addWidget(myStatusLabel, 1);
    myStatusProgress = new QProgressBar();
    myUi->statusbar->addWidget(myStatusProgress, 0);
    myStatusProgress->setRange(0, 100);
    myStatusProgress->hide();

    myUi->graphicsView->setViewport(new QGLWidget);
    myUi->graphicsView->setScene(new QGraphicsScene);
    myUi->graphicsView->setRenderHints(QPainter::HighQualityAntialiasing);
    myUi->graphicsView->setInteractive(true);

    myGame.reset(new Game(myEngine));

    reloadData();
  }

  Editor::~Editor()
  {
  }

  void Editor::reloadData()
  {
    myEngine->service().post([&] {
      myGame->reloadData();
      emit onDataReloaded();
    });
  }

  void Editor::dataChanged()
  {
    if (boost::filesystem::exists(dataDir / "save" / "index") &&
        boost::filesystem::file_size(dataDir / "save" / "index"))
    {
      myUi->action_Create_Map->setEnabled(false);
      myUi->action_Load_Map->setEnabled(true);
    }
    else
    {
      myUi->action_Create_Map->setEnabled(true);
      myUi->action_Load_Map->setEnabled(false);
    }
    myUi->action_Save->setEnabled(true);
    myUi->action_Save_As->setEnabled(true);
  }

  void Editor::createMap()
  {
    myEngine->scheduler()->schedule([&] {
      myGame->createMap();
      emit onMapReady();
    });
    myStatusProgress->show();
    myStatusLabel->setText("Preprocessing map, please wait..");
    if (myProgressTimer) delete myProgressTimer;
    myProgressTimer = new QTimer;
    QObject::connect(myProgressTimer, SIGNAL(timeout()), this, SLOT(updateProgress()));
    myProgressTimer->start(100);
  }

  void Editor::updateProgress()
  {
    if (!myGame->generator()) return;
    int progress = myGame->generator()->preprocessingProgress();
    myStatusProgress->setValue(progress);
    if (progress >= 100)
    {
      myStatusLabel->setText("Ready");
      myStatusProgress->hide();
      myProgressTimer->stop();
      myUi->action_Create_Map->setEnabled(false);
      myUi->action_Load_Map->setEnabled(false);
      myUi->action_Unload_Map->setEnabled(true);
      myUi->action_Terrain_Parameters->setEnabled(true);
      emit onMapReady();
    }
  }

  void Editor::refreshMap()
  {
    myUi->graphicsView->scene()->setSceneRect(0, 0, myGame->generator()->width(), myGame->generator()->height());
    for (unsigned int i = 0; i < myGame->generator()->regions().size(); i++)
    {
      QPolygonF qring, sqring;
      auto ring = boost::geometry::exterior_ring(myGame->generator()->regions()[i].poly);
      boost::geometry::convert(ring, qring);
      boost::geometry::simplify(qring, sqring, 1);
      QPen pen;
      pen.setColor(QColor(0,0,0));
      pen.setStyle(Qt::PenStyle::DashLine);
      QBrush brush;
      brush.setStyle(Qt::BrushStyle::SolidPattern);
      std::string colour = myGame->biomes()[myGame->generator()->regions()[i].biome]->jsonValue["mapColour"].asString();
      pen.setColor(QColor(colour.c_str()));
      brush.setColor(QColor(colour.c_str()));
      QGraphicsPolygonItem * pi = new RegionPolygonItem(this);
      pi->setPen(pen);
      pi->setBrush(brush);
      pi->setPolygon(sqring);
      pi->setZValue(1/myGame->generator()->regions()[i].area);
      pi->setData(0, i);
      pi->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsFocusable, true);
      pi->setFlag(QGraphicsItem::GraphicsItemFlag::ItemIsSelectable, true);
      myUi->graphicsView->scene()->addItem(pi);
    }
  }

  void Editor::onMessage(LogLevel level, const QString & source, const QString & message)
  {
    QString slevel;
    switch (level)
    {
      case LogLevel::Debug: slevel = "Debug"; break;
      case LogLevel::Info:  slevel = "Info"; break;
      case LogLevel::Error: slevel = "Error"; break;
      case LogLevel::Fatal: slevel = "Fatal"; break;
    }
    int index = myUi->tableLog->rowCount();
    if (index > 0 &&
        level == myUi->tableLog->item(index-1, 0)->data(Qt::UserRole) &&
        source == myUi->tableLog->item(index-1, 1)->text() &&
        message == myUi->tableLog->item(index-1, 2)->data(Qt::UserRole))
    {
      myUi->tableLog->item(index-1, 2)->setText(message +
        " (x" + QVariant(myUi->tableLog->item(index-1, 1)->data(Qt::UserRole).toInt() + 1).toString() + ")");
      myUi->tableLog->item(index-1, 1)->setData(Qt::UserRole, myUi->tableLog->item(index-1, 2)->data(1).toInt() + 1);
    }
    else
    {
      myUi->tableLog->setRowCount(index + 1);
      myUi->tableLog->setItem(index, 0, new QTableWidgetItem(slevel));
      myUi->tableLog->setItem(index, 1, new QTableWidgetItem(source));
      myUi->tableLog->setItem(index, 2, new QTableWidgetItem(message));
      myUi->tableLog->item(index, 0)->setData(Qt::UserRole, level);
      myUi->tableLog->item(index, 1)->setData(Qt::UserRole, 1);
      myUi->tableLog->item(index, 2)->setData(Qt::UserRole, message);
      myUi->tableLog->scrollToBottom();
      myUi->tableLog->resizeColumnToContents(0);
      myUi->tableLog->resizeColumnToContents(1);
    }
  }

  void Editor::showHeightMapEditor()
  {
    HeightMapEditor hmeditor(this);
    hmeditor.setAttribute(Qt::WA_ShowModal);
    hmeditor.setWindowFlags(Qt::WindowFlags(Qt::WindowType::Dialog));
    hmeditor.setWindowModality(Qt::WindowModality::ApplicationModal);
    hmeditor.show();
    while (hmeditor.isVisible())
    {
      QApplication::processEvents();
      QApplication::sendPostedEvents();
    }
  }
}

