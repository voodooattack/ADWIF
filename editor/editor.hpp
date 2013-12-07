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

#ifndef EDITOR_H
#define EDITOR_H

#include <QtGui/QMainWindow>
#include <QtGui/QTableWidget>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>

#include <memory>
#include "engine.hpp"

namespace Ui
{
  class Editor;
}

namespace ADWIF
{
  class Game;

  class EditorLogProvider: public QObject, public LogProvider
  {
    Q_OBJECT
  public:
    EditorLogProvider() { }
    virtual ~EditorLogProvider() { }

    virtual void logMessage(LogLevel level, const std::string & source, const std::string & message)
    {
      emit onMessage(level, source.c_str(), message.c_str());
    }

  signals:
    void onMessage(LogLevel level, const QString & source, const QString & message);
  };

  class ProgressWindow: public QWidget
  {
    Q_OBJECT

  public:
    explicit ProgressWindow(QWidget * parent = 0, Qt::WindowFlags f = 0): QWidget(parent, f)
    {
      this->setWindowModality(Qt::WindowModality::WindowModal);
    }

    virtual ~ProgressWindow() { }

  };

  class Editor: public QMainWindow
  {
    Q_OBJECT

  public:
    Editor(const std::shared_ptr<class Engine> & engine);
    virtual ~Editor();

  public slots:
    void reloadData();
    void createMap();
    void refreshMap();

    void dataChanged();
    void updateProgress();
    void onMessage(LogLevel level, const QString & source, const QString & message);

  signals:
    void onDataReloaded();
    void onMapReady();
    void onMapProgress(double progress);

  private:
    std::shared_ptr<Engine> myEngine;
    std::shared_ptr<Game> myGame;
    std::shared_ptr<Ui::Editor> myUi;
    QLabel * myStatusLabel;
    QProgressBar * myStatusProgress;
    QTimer * myProgressTimer;
  };
}

#endif // EDITOR_H
