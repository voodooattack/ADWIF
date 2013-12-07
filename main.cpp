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

#include "config.hpp"

#include <signal.h>
#include <physfs.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "engine.hpp"
#include "fileutils.hpp"

#include "map.hpp"
#include "introstate.hpp"

#ifdef ADWIF_BUILD_EDITOR
#include "editorstate.hpp"
#endif

#include "util.hpp"

#ifdef ADWIF_RENDERER_USE_CURSES
#include "curses.hpp"
#elif defined(ADWIF_RENDERER_USE_TCOD)
#include "tcod.hpp"
#endif

namespace ADWIF
{
  boost::program_options::variables_map options;
}

using namespace ADWIF;
namespace po = boost::program_options;

/* void sig_handler(int code)
{

} */

int main(int argc, char ** argv)
{
  //signal(SIGINT, sig_handler);      /* arrange interrupts to terminate */

  std::ios_base::sync_with_stdio (false);

//   setlocale(LC_CTYPE, "");
  setlocale(LC_ALL, "en_US.UTF-8");

  PhysFS::init(argv[0]);

  po::options_description odesc("Usage");

  odesc.add_options()
#ifdef ADWIF_UNICODE
    ("unicode", "start in Unicode mode (only use this if your terminal and font support it)")
#endif
#ifdef ADWIF_BUILD_EDITOR
    ("editor", "start in game editor mode")
#endif
    ("help", "show this help message");

  po::store(po::parse_command_line(argc, argv, odesc), options);
  po::notify(options);

  if (options.count("help")) {
    std::cout << odesc << "\n";
    return 1;
  }

  writeDir = boost::filesystem::path(PhysFS::getUserDir()) / ".adwif";
  dataDir = boost::filesystem::path(PhysFS::getBaseDir()) / "data";
  dataFile = boost::filesystem::path(PhysFS::getBaseDir()) / "data.dat";
  saveDir = writeDir / "save";

  boost::filesystem::create_directory(writeDir);
  boost::filesystem::create_directory(saveDir);

  PhysFS::setWriteDir(writeDir.native());
  PhysFS::mount(dataDir.native(), "/", false);
  PhysFS::mount(dataFile.native(), "/", true);

  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<Input> input;
  std::shared_ptr<Engine> engine;

#ifdef ADWIF_RENDERER_USE_CURSES
  renderer.reset(new CursesRenderer());
  input.reset(new CursesInput(renderer));
#elif defined(ADWIF_RENDERER_USE_TCOD)
  renderer.reset(new TCODRenderer());
  input.reset(new TCODInput(renderer));
#endif

  if (!renderer->init())
  {
    std::cerr << "Error initialising the display system." << std::endl;
    return 1;
  }

  if (!input->init())
  {
    std::cerr << "Error initialising the input system." << std::endl;
    return 1;
  }

  engine.reset(new Engine(renderer, input));

  std::shared_ptr<GameState> state;


#ifdef ADWIF_BUILD_EDITOR
  if (options.count("editor")) {
    state.reset(new EditorState(engine, argc, argv));
  }
  else
#endif
    state.reset(new IntroState(engine));

  engine->addState(state);

  int ret = engine->start();

  PhysFS::deinit();

  return ret;
}

