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

#ifndef HEIGHTMAPEDITOR_H
#define HEIGHTMAPEDITOR_H

#include <QWidget>
#include <QImage>

#include <vector>
#include <map>

#include "editor.hpp"

namespace Ui
{
  class HeightMapEditor;
}

namespace noise { namespace module { class Module; } }

namespace ADWIF
{
  struct NoiseGraph
  {
    std::vector<std::shared_ptr<noise::module::Module>> modules;
    std::map<std::string, std::shared_ptr<noise::module::Module>> defs;
    std::shared_ptr<noise::module::Module> module;
  };

  class AreaGenerationTask: public QObject
  {
    Q_OBJECT
  public:
    AreaGenerationTask(const std::shared_ptr<class Engine> & engine,
                           const std::shared_ptr<NoiseGraph> & graph,
                           const QRectF & area):
      engine(engine), graph(graph), area(area), priority(-1),
      cancellationFlag(false), completeFlag(false),
      image(area.size().toSize(), QImage::Format_ARGB32)
    {
    }
    void operator()();
  signals:
    void generationCompleted(QRectF area, QImage image);
    void cancelled(QRectF area);
  public:
    std::shared_ptr<NoiseGraph> graph;
    std::shared_ptr<class Engine> engine;
    QRectF area;
    boost::atomic_int priority;
    boost::atomic_bool cancellationFlag, completeFlag;
    QImage image;
  };

  class HeightMapEditor: public QWidget
  {
    Q_OBJECT
  public:
    explicit HeightMapEditor(Editor * parent = 0, Qt::WindowFlags f = 0);
    virtual ~HeightMapEditor() { }

    std::shared_ptr<class Engine> engine() const { return myEngine; }
    void setEngine(const std::shared_ptr<class Engine> & engine) { myEngine = engine; }

  public slots:
    void onRenderButtonClicked();
    void onShowSrcButtonClicked();
    void onAreaGenerated(QRectF area, QImage image);
    void onViewChanged(const QRectF & rect);
    void generateRect(const QRectF & rect);
  private:
    QSharedPointer<Ui::HeightMapEditor> myUi;
    QSizeF myCellSize;
    std::shared_ptr<class Engine> myEngine;
    std::shared_ptr<NoiseGraph> myGraph;
    QList<std::shared_ptr<AreaGenerationTask>> myTasks;
    Editor * myEditor;
  };
}

#endif // HEIGHTMAPEDITOR_H
