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
    QObject::connect(myUi->renderView, SIGNAL(viewChanged(QRectF)), this, SLOT(onViewChanged(QRectF)));

//     myUi->renderView->setViewport(new QGLWidget(myUi->renderView));
    myUi->renderView->setScene(new QGraphicsScene(myUi->renderView));
    myUi->renderView->setRenderHints(QPainter::Antialiasing);
    myUi->renderView->scene()->clear();
//     myUi->renderView->scene()->setSceneRect(QRectF(-800, -800, 1600, 1600));

    myUi->splitterMain->setSizes({ myUi->splitterSub->minimumWidth(),
      geometry().width() - myUi->splitterSub->minimumWidth() });
  }

  void HeightMapEditor::onRenderButtonClicked()
  {
    for (auto task : myTasks)
      task->cancellationFlag.store(true);
    myTasks.clear();
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
    } catch (ParsingException & e) {
      QMessageBox::critical(this, "Error", e.what());
      return;
    }

    generateRect(myUi->renderView->sceneRect());
  }

  void HeightMapEditor::onAreaGenerated(QRectF area, QImage image)
  {
    myUi->renderView->scene()->addPixmap(QPixmap::fromImage(image, Qt::ColorOnly))->setPos(area.topLeft());
    myUi->renderView->scene()->update(area);
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
      myUi->renderView->scene()->clear();

    QRectF trect;

    trect.setLeft(rect.left() - fmod(rect.left(), myCellSize.width()));
    trect.setTop(rect.top() - fmod(rect.top(), myCellSize.height()));
    trect.setRight(rect.right() - fmod(rect.right(), myCellSize.width()));
    trect.setBottom(rect.bottom() - fmod(rect.bottom(), myCellSize.height()));

    for (double y = trect.top(); y < trect.bottom(); y += myCellSize.height())
      for (double x = trect.left(); x < trect.right(); x += myCellSize.width())
      {
        QRectF r(x,y,myCellSize.width(), myCellSize.height());
        generateRect(r);
      }
  }

  void HeightMapEditor::generateRect(const QRectF & rect)
  {
    for (auto task = myTasks.begin(); task != myTasks.end(); task++)
    {
      if ((*task)->cancellationFlag || (*task)->completeFlag)
        continue;
      if ((*task)->area.intersects(rect)) {
        (*task)->priority = 0;
        return;
      }
      else (*task)->priority = 2;
    }

    std::shared_ptr<AreaGenerationTask> task(new AreaGenerationTask(myEngine, myGraph, rect));

    task->priority = 0;
    QObject::connect(task.get(), SIGNAL(generationCompleted(QRectF, QImage)),
                     this, SLOT(onAreaGenerated(QRectF, QImage)), Qt::QueuedConnection);

    myEngine->scheduler()->schedule(boost::bind(&AreaGenerationTask::operator(), task));
    myTasks.push_back(task);
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
        QColor col(height * 255.0, height * 255.0, height * 255.0);
        image.setPixel(x - area.left(), y - area.top(), col.rgb());
      }
      if (priority != 0)
      {
        if (counter % priority.load() == 0)
          engine->scheduler()->yield();
      }
    }
    if (!cancellationFlag)
      emit generationCompleted(area, image);
    else
      emit cancelled(area);
    completeFlag.store(true);
  }
}

