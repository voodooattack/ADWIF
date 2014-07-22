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

#include "heightmapeditor.hpp"
#include "ui_heightmapeditor.h"

#include "noiseutils.hpp"
#include <physfs.hpp>
#include <jsonutils.hpp>

#include <QTextEdit>
#include <QBoxLayout>
#include <QtOpenGL/QGLWidget>
#include <QTextEdit>
#include <QStandardItemModel>
#include <QGraphicsItem>
#include <QMessageBox>

namespace ADWIF
{
  HeightMapEditor::HeightMapEditor(ADWIF::Editor * parent, Qt::WindowFlags f):
    QWidget(parent, f), myEditor(parent), myCellSize(200,200)
  {
    myUi = myUi.create();
    myUi->setupUi(this);

    myUi->graphBuilder->setPropertyBrowser(myUi->propertyBrowser);

    layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

    QObject::connect(myUi->buttonRender, SIGNAL(clicked()), this, SLOT(onRenderButtonClicked()));
    QObject::connect(myUi->buttonShowSrc, SIGNAL(clicked()), this, SLOT(onShowSrcButtonClicked()));
    QObject::connect(myUi->renderView, SIGNAL(onViewChanged(QRectF)), this, SLOT(onViewChanged(QRectF)));

    myUi->renderView->setCellSize(myCellSize);
    myUi->renderView->setScene(new QGraphicsScene(myUi->renderView));
    myUi->renderView->setRenderHints(QPainter::RenderHint::SmoothPixmapTransform);
    QBrush sceneBrush(Qt::lightGray, Qt::BrushStyle::BDiagPattern);
    myUi->renderView->scene()->setBackgroundBrush(sceneBrush);
    myUi->renderView->scene()->clear();

    myUi->splitterMain->setSizes({ myUi->splitterSub->minimumWidth(),
      geometry().width() - myUi->splitterSub->minimumWidth() });

    PhysFS::ifstream fs("map/heightgraph.json");
    std::string json;
    json.assign(std::istreambuf_iterator<std::string::value_type>(fs),
                std::istreambuf_iterator<std::string::value_type>());
    Json::Value value;
    Json::Reader reader;

    if (reader.parse(json, value))
    {
      myUi->graphBuilder->fromJson(value);
    }
  }

  HeightMapEditor::~HeightMapEditor()
  {
    boost::recursive_mutex::scoped_lock guard(myMutex);
    for (auto task : myTasks)
      task->cancellationFlag.store(true);
    myTasks.clear();
  }

  void HeightMapEditor::onRenderButtonClicked()
  {
    {
      boost::recursive_mutex::scoped_lock guard(myMutex);
      for (auto task : myTasks)
        task->cancellationFlag.store(true);
      myTasks.clear();
    }
    myUi->renderView->scene()->clear();
    Json::Value graphJson = myUi->graphBuilder->toJson();
    myGraph.reset(new NoiseGraph);
    PhysFS::ifstream fs("map/heightmap.png");
    std::vector<char> data;
    data.assign(std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());
    QImage heightmapImage;
    heightmapImage.loadFromData((const uchar *)data.data(), data.size());
    boost::multi_array<double, 2> heights;
    heights.resize(boost::extents[heightmapImage.width()][heightmapImage.height()]);
    for (int y = 0; y < heightmapImage.height(); y++)
      for (int x = 0; x < heightmapImage.width(); x++)
      {
        QColor col(heightmapImage.pixel(x, y));
        int height = col.red() | col.green() | col.blue();
        heights[x][y] = (height / 256.0) * 2 - 1;
      }
    std::shared_ptr<HeightMapModule> heightmap(new HeightMapModule(heights, 400, 240));
    try {
      myGraph->module = buildNoiseGraph(graphJson, myGraph->modules, myGraph->defs, heightmap, 0);
    } catch (std::exception & e) {
      QMessageBox::critical(this, "Error", e.what());
      myGraph.reset();
      return;
    } catch (noise::ExceptionInvalidParam & e) {
      QMessageBox::critical(this, "Error", "Invalid parameter");
      myGraph.reset();
      return;
    } catch (noise::ExceptionNoModule & e) {
      QMessageBox::critical(this, "Error", "Missing module");
      myGraph.reset();
      return;
    } catch (noise::ExceptionOutOfMemory & e) {
      QMessageBox::critical(this, "Error", "Out of memory");
      myGraph.reset();
      return;
    } catch (noise::ExceptionUnknown & e) {
      QMessageBox::critical(this, "Error", "Unknown exception");
      myGraph.reset();
      return;
    }

    generateRect(myUi->renderView->sceneRect());
  }

  void HeightMapEditor::onAreaGenerated(const QRectF & area, const QImage & image)
  {
    myUi->renderView->scene()->addPixmap(QPixmap::fromImage(image, Qt::AutoColor))->setPos(area.topLeft());
    myUi->renderView->scene()->update(area);
    boost::recursive_mutex::scoped_lock guard(myMutex);
    for (auto task = myTasks.begin(); task != myTasks.end(); task++)
    {
      if ((*task)->cancellationFlag || (*task)->completeFlag)
      {
        task = myTasks.erase(task);
        continue;
      }
    }
  }

  void HeightMapEditor::onShowSrcButtonClicked()
  {
    Json::Value graphJson = myUi->graphBuilder->toJson();
    QWidget * window = new QWidget(this);
    QVBoxLayout * layout = new QVBoxLayout(window);
    QTextEdit * edit = new QTextEdit(window);
    layout->addWidget(edit);
    window->setLayout(layout);
    window->setWindowFlags(Qt::WindowFlags(Qt::WindowType::Dialog));
    window->setAttribute(Qt::WA_DeleteOnClose);
    edit->setText(graphJson.toStyledString().c_str());
    window->show();
  }

  void HeightMapEditor::onViewChanged(const QRectF & rect)
  {
    if (!myGraph) return;
    if (myUi->renderView->scene()->children().count() > 50)
    {
      QList<QGraphicsItem *> items = myUi->renderView->scene()->items();
      for (QGraphicsItem * i : items)
        if (!i->isVisible())
          myUi->renderView->scene()->removeItem(i);
    }

    QRectF trect;

    trect.setLeft(rect.left() - fmod(rect.left(), myCellSize.width()));
    trect.setTop(rect.top() - fmod(rect.top(), myCellSize.height()));
    trect.setRight(rect.right() - fmod(rect.right(), myCellSize.width()));
    trect.setBottom(rect.bottom() - fmod(rect.bottom(), myCellSize.height()));

    for (double y = trect.top() - fmod(trect.top(), myCellSize.height());
         y < trect.bottom() + myCellSize.height() - fmod(trect.bottom(), myCellSize.height());
         y += myCellSize.height())
      for (double x = trect.left() - fmod(trect.left(), myCellSize.width());
           x < trect.right() + myCellSize.width() - fmod(trect.right(), myCellSize.width());
           x += myCellSize.width())
      {
        QRectF r(x,y,myCellSize.width(), myCellSize.height());
        generateRect(r);
      }
  }

  void HeightMapEditor::generateRect(const QRectF & rect)
  {

    QRegion region(rect.toRect(), QRegion::RegionType::Rectangle);

    for (QGraphicsItem * i : myUi->renderView->scene()->items())
      region -= i->boundingRegion(i->sceneTransform());

    if (region.isEmpty()) return;

    {
      boost::recursive_mutex::scoped_lock guard(myMutex);
      for (auto task = myTasks.begin(); task != myTasks.end(); task++)
      {
        if ((*task)->cancellationFlag || (*task)->completeFlag)
          continue;
        if ((*task)->area.intersects(rect))
        {
          QRegion sub((*task)->area.toRect(), QRegion::RegionType::Rectangle);
          region -= sub;
          if (region.rectCount() > 1)
            for (auto r : region.rects())
              generateRect(r);
          else
            (*task)->priority = 0;
          return;
        }
      }
    }

    std::shared_ptr<AreaGenerationTask> task(new AreaGenerationTask(myEngine, myGraph, rect));

    task->priority = 0;
    QObject::connect(task.get(), SIGNAL(generationCompleted(QRectF, QImage)),
                     this, SLOT(onAreaGenerated(QRectF, QImage)), Qt::QueuedConnection);

    myEngine->scheduler()->schedule((std::bind(&AreaGenerationTask::operator(), task)));
    myTasks.push_back(task);
  }

  AreaGenerationTask::AreaGenerationTask(const std::shared_ptr< Engine > & engine,
                                         const std::shared_ptr< NoiseGraph > & graph,
                                         const QRectF & area) :
    engine(engine), graph(graph), area(area), priority(-1),
    cancellationFlag(false), completeFlag(false),
    image(area.size().toSize(), QImage::Format_Indexed8)
  {
    image.setColorCount(256);
    for (int i = 0; i < 256; i++)
      image.setColor(i, QColor(i, i, i, 255).rgba());
  }

  void AreaGenerationTask::operator()()
  {
    int counter = 0;
    for (double y = area.top(); y < area.bottom(); y++)
    {
      counter++;
      for (double x = area.left(); x < area.right(); x++)
      {
        if (cancellationFlag)
          break;
        double value = graph->module->GetValue(x, y, 0.0);
        double height = (value + 1.0) * 0.5;
        if (height < 0) height = 0;
        if (height > 1) height = 1;
        image.setPixel(floor(x - area.left()), floor(y - area.top()), qRound(height * 255.0));
      }
      if (priority != 0)
        if (counter % priority.load() == 0)
          engine->scheduler()->yield();
    }
    if (!cancellationFlag)
      emit generationCompleted(area, image);
    else
      emit cancelled(area);
    completeFlag.store(true);
  }
}

