/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "GuiHelperAction.h"
#include <iostream>

namespace Isis {
  GuiHelperAction::GuiHelperAction(QObject *parent, const QString &funct) : QAction(parent) {
    p_funct = funct;
    connect(this, SIGNAL(triggered(bool)), this, SLOT(retrigger()));
  }

  GuiHelperAction::~GuiHelperAction() {};

  void GuiHelperAction::retrigger() {
    emit trigger(p_funct);
  }

}

