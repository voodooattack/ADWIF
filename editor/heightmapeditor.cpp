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
#include <qt4/QtGui/QTextEdit>
#include <qt4/QtGui/QBoxLayout>

#include <QtOpenGL/QGLWidget>
#include <QTextEdit>
#include <QStandardItemModel>

namespace ADWIF
{
  HeightMapEditor::HeightMapEditor(ADWIF::Editor * parent, Qt::WindowFlags f): QWidget(parent, f), myEditor(parent)
  {
    myUi = myUi.create();
    myUi->setupUi(this);

    myUi->renderView->setViewport(new QGLWidget);
    myUi->renderView->setScene(new QGraphicsScene);
    myUi->renderView->setRenderHints(QPainter::HighQualityAntialiasing);
    myUi->renderView->setScene(new QGraphicsScene(myUi->renderView));

    myUi->graphBuilder->setPropertyBrowser(myUi->propertyBrowser);

    layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

    QObject::connect(myUi->buttonRender, SIGNAL(clicked()), this, SLOT(onRenderButtonClicked()));
    QObject::connect(myUi->buttonShowSrc, SIGNAL(clicked()), this, SLOT(onShowSrcButtonClicked()));
  }

  void HeightMapEditor::onRenderButtonClicked()
  {
    Json::Value graphJson = myUi->graphBuilder->toJson();
    std::cerr << graphJson.toStyledString() << std::endl;
    std::vector<std::shared_ptr<noise::module::Module>> modules;
    std::map<std::string, std::shared_ptr<noise::module::Module>> defs;
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
    std::shared_ptr<noise::module::Module> graph = buildNoiseGraph(graphJson, modules, defs, heightmap, 0);
    QImage img(400 * 4, 240*4, QImage::Format::Format_ARGB32);
    for (int y = 0; y < img.height(); y++)
      for (int x = 0; x < img.width(); x++)
      {
        double value = graph->GetValue((double)x, (double)y, 0.0);
        double height = (value + 1.0) * 0.5;
//         std::cerr << value << std::endl;
        if (height < 0) height = 0;
        if (height > 1) height = 1;
        QColor col (height * 255.0, height * 255.0, height * 255.0);
        img.setPixel(x, y, col.rgb());
      }
    myUi->renderView->scene()->clear();
    myUi->renderView->scene()->addPixmap(QPixmap::fromImage(img, Qt::ColorOnly));
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
}

