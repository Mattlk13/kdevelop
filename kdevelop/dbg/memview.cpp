/***************************************************************************
                          disassemble.cpp  -  description
                             -------------------
    begin                : Tue Oct 5 1999
    copyright            : (C) 1999 by John Birch
    email                : jb.nz@writeme.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "memview.h"

#include <kapp.h>
#include <kbuttonbox.h>
#include <klined.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qmultilinedit.h>

// **************************************************************************
//
// Dialog allows the user to enter
//  - A starting address
//  - An ending address
//
//  this can be in the form
//            functiom/method name
//            variable address (ie &Var, str)
//            Memory address 0x8040abc
//
//  When disassembling and you enter a method name without an
//  ending address then the whole method is disassembled.
//  No data means disassemble the method we're curently in.(from the
//  start of the method)
//
// click ok buton to send the request to gdb
// the output is returned (some time later) in the raw data slot
// and displayed as is, so it's rather crude, but it works!
// **************************************************************************

MemoryView::MemoryView(QWidget *parent, const char *name) :
  QDialog(parent, name, true),      // modal
  start_(new KLined(this)),
  end_(new KLined(this)),
  output_(new QMultiLineEdit(this))
{
  // Make the top-level layout; a vertical box to contain all widgets
  // and sub-layouts.
  QBoxLayout *topLayout = new QVBoxLayout(this);

  QGridLayout *grid = new QGridLayout(2, 2, 10);
  topLayout->addLayout(grid);

  QLabel *label;
  label = new QLabel(this);
  label->setText(i18n("Start:"));
  label->setMaximumHeight(label->sizeHint().height());
  label->setMinimumSize(label->sizeHint());
  grid->addWidget(label, 0, 0);
  grid->setRowStretch(0, 0);

  label->setBuddy(start_);
  start_->setMaximumHeight(label->sizeHint().height());
  start_->setMinimumSize(label->sizeHint());
  grid->addWidget(start_, 1, 0);
  grid->setRowStretch(1, 0);

  label = new QLabel(this);
  label->setText(i18n("Amount/End address (memory/disassemble):"));
  label->setMaximumHeight(label->sizeHint().height());
  label->setMinimumSize(label->sizeHint());
  grid->addWidget(label, 0, 1);

  label->setBuddy(end_);
  end_->setMaximumHeight(label->sizeHint().height());
  end_->setMinimumSize(label->sizeHint());
  grid->addWidget(end_, 1, 1);

  label = new QLabel(this);
  label->setText(i18n("MemoryView:"));
  label->setMaximumHeight(label->sizeHint().height());
  label->setMinimumSize(label->sizeHint());
  topLayout->addWidget(label, 0);
  topLayout->addWidget(output_, 10);

  KButtonBox *buttonbox = new KButtonBox(this);
  QPushButton *memoryDump = buttonbox->addButton(i18n("Memory"));
  QPushButton *disassemble = buttonbox->addButton(i18n("Disassemble"));
  QPushButton *registers = buttonbox->addButton(i18n("Registers"));
  QPushButton *libraries = buttonbox->addButton(i18n("Libraries"));
  QPushButton *cancel = buttonbox->addButton(i18n("Cancel"));
  connect(memoryDump, SIGNAL(clicked()), SLOT(slotMemoryDump()));
  connect(disassemble, SIGNAL(clicked()), SLOT(slotDisassemble()));
  connect(registers, SIGNAL(clicked()), SIGNAL(registers()));
  connect(libraries, SIGNAL(clicked()), SIGNAL(libraries()));
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));
  memoryDump->setDefault(true);
  buttonbox->layout();
  topLayout->addWidget(buttonbox);

  topLayout->activate();
}

// **************************************************************************

MemoryView::~MemoryView()
{
}

// **************************************************************************

void MemoryView::slotRawGDBMemoryView(char* buf)
{
  // just display the resultant output from GDB in the edit box
  output_->clear();
  output_->insertLine(buf);
  output_->setCursorPosition(0,0);
}

// **************************************************************************

// get gdb to supply the disassembled data.
void MemoryView::slotDisassemble()
{
  QString start(start_->text());
  QString end(end_->text());
  emit disassemble(start, end);
}

// **************************************************************************

void MemoryView::slotMemoryDump()
{
  QString start(start_->text());
  QString size(end_->text());
  emit memoryDump(start, size);
}

// **************************************************************************
// **************************************************************************
// **************************************************************************
