/***************************************************************************
                          brkptmanager.cpp  -  description                              
                             -------------------                                         
    begin                : Sun Aug 8 1999                                           
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

#include "brkptmanager.h"
#include "breakpoint.h"

#include <qlistbox.h>
#include <qdict.h>
#include <qheader.h>

#include <kpopmenu.h>
#include <kapp.h>

#include <stdlib.h>
#include <ctype.h>

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

BreakpointManager::BreakpointManager(QWidget* parent, const char* name, WFlags f) :
  QListBox(parent, name, f),
  activeFlag_(0)
{
  setCaption("Breakpoint manager");
}

/***************************************************************************/

BreakpointManager::~BreakpointManager()
{
}

/***************************************************************************/

void BreakpointManager::reset()
{
  for (int index=0; index<(int)count(); index++)
    ((Breakpoint*)item(index))->reset();
  repaint();
}

/***************************************************************************/

void BreakpointManager::removeAll()
{
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)item(index);
    BP->setActionDie();
    emit publishBPState(BP);
    removeItem(index);
  }

  repaint();
}

/***************************************************************************/

// Essentially tells the editor (display window) what lines contain a
// breakpoint. Used when a file is loaded
void BreakpointManager::refreshBP(const QString& filename)
{
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)item(index);
    if (BP->hasSourcePosition() && (BP->filename() == filename))
      emit refreshBPState(BP);
  }
}

/***************************************************************************/

int BreakpointManager::findIndex(const Breakpoint* breakpoint) const
{
  // NOTE:- The match doesn't have to be equal. Each type of BP
  // must decide on the match criteria.
  ASSERT (breakpoint);

  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)(item(index));
    if (breakpoint->match(BP))
      return index;
  }

  return -1;
}

/***************************************************************************/

// The Id is supplied by the debugger
Breakpoint* BreakpointManager::findId(int dbgId) const
{
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)item(index);
    if (BP->dbgId() == dbgId)
      return BP;
  }

  return 0;
}

/***************************************************************************/

// The key is a unique number supplied by us
Breakpoint* BreakpointManager::findKey(int BPKey) const
{
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)item(index);
    if (BP->key() == BPKey)
      return BP;
  }

  return 0;
}


/***************************************************************************/

void BreakpointManager::addBreakpoint(Breakpoint* BP)
{
  insertItem(BP);
  BP->setActionAdd(true);
  BP->setPending(true);
  emit publishBPState(BP);

  BP->configureDisplay();
  repaint();
}

/***************************************************************************/

void BreakpointManager::removeBreakpoint(int index)
{
  Breakpoint* BP = (Breakpoint*)item(index);

  // Pending but the debugger hasn't started processing this BP so
  // we can just remove it.
  if (BP->isPending() && !BP->isDbgProcessing())
  {
    BP->setActionDie();
    emit publishBPState(BP);
    removeItem(index);
  }
  else
  {
    BP->setPending(true);
    BP->setActionClear(true);
    emit publishBPState(BP);

    BP->configureDisplay();
  }

  repaint();
}

/***************************************************************************/

void BreakpointManager::modifyBreakpoint(int index)
{
  Breakpoint* BP = (Breakpoint*)item(index);
  if (BP->modifyDialog())
  {
    BP->setPending(true);
    BP->setActionModify(true);
    emit publishBPState(BP);

    BP->configureDisplay();
    repaint();
  }
}

/***************************************************************************/

void BreakpointManager::mouseReleaseEvent(QMouseEvent* e)
{
  QListBox::mouseReleaseEvent(e);

  if ((e->state() == RightButton) && (findItem(e->pos().y()) != -1))
    if (currentItem() >= 0)
      breakpointPopup((Breakpoint*)item(currentItem()));
}

/***************************************************************************/

void BreakpointManager::mouseDoubleClickEvent(QMouseEvent *e)
{
  QListBox::mouseDoubleClickEvent(e);
  slotGotoBreakpointSource();
}

/***************************************************************************/

void BreakpointManager::breakpointPopup(Breakpoint* BP)
{
  KPopupMenu popup("Breakpoint menu");
  popup.insertItem( i18n("Remove breakpoint"),    this, SLOT(slotRemoveBreakpoint()) );
  popup.insertItem( i18n("Modify breakpoint"),    this, SLOT(slotModifyBreakpoint()) );
  if (BP->hasSourcePosition())
    popup.insertItem( i18n("Display source code"),  this, SLOT(slotGotoBreakpointSource()) );
  popup.exec(QCursor::pos());
}

/***************************************************************************/

void BreakpointManager::slotRemoveBreakpoint()
{
  if (currentItem() >= 0)
    removeBreakpoint(currentItem());
}

/***************************************************************************/

void BreakpointManager::slotModifyBreakpoint()
{
  if (currentItem() >= 0)
    modifyBreakpoint(currentItem());
}

/***************************************************************************/

void BreakpointManager::slotGotoBreakpointSource()
{
  if (currentItem() >= 0)
  {
    Breakpoint* BP = (Breakpoint*)item(currentItem());
    if (BP->hasSourcePosition())
      emit gotoSourcePosition(BP->filename(), BP->lineNo()-1);
  }
}

/***************************************************************************/

void BreakpointManager::slotSetPendingBPs()
{
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)(item(index));
		if (BP->isPending() && !BP->isDbgProcessing())
      emit publishBPState(BP);
  }
}

/***************************************************************************/

// The debugger is having trouble with this BP - probably because a library
// was unloaded and invalidated a BP that was previously set in the library
// code. Reset the BP so that we can try again later.
void BreakpointManager::slotUnableToSetBPNow(int BPid)
{
  if (BPid == -1)
    reset();
  else
    if (Breakpoint* BP = findId(BPid))
      BP->reset();

  repaint();
}

/***************************************************************************/

void BreakpointManager::slotToggleStdBreakpoint
              (const QString& fileName, int lineNo, bool RMBClicked)
{
  FilePosBreakpoint* fpBP = new FilePosBreakpoint(fileName, lineNo);

  int found = findIndex(fpBP);
  if (found >= 0)
  {
    delete fpBP;
    if (RMBClicked)
      modifyBreakpoint(found);
    else
      removeBreakpoint(found);
  }
  else
    addBreakpoint(fpBP);
}

/***************************************************************************/

void BreakpointManager::slotToggleWatchpoint(const QString& varName)
{
	Watchpoint* watchpoint = new Watchpoint(varName, false, true);
  int found = findIndex(watchpoint);
  if (found >= 0)
  {
    removeBreakpoint(found);
    delete watchpoint;
  }
  else
    addBreakpoint(watchpoint);
}

/***************************************************************************/

void BreakpointManager::slotParseGDBBrkptList(char* str)
{
// An example of a GDB breakpoint table
//Num Type           Disp Enb Address    What
//1   breakpoint     del  y   0x0804a7fb in main at main.cpp:22
//2   hw watchpoint  keep y   thisIsAGlobal_int
//3   breakpoint     keep y   0x0804a847 in main at main.cpp:23
//        stop only if thisIsAGlobal_int == 1
//        breakpoint already hit 1 time
//4   breakpoint     keep y   0x0804a930 in main at main.cpp:28
//        ignore next 6 hits

// Another example of a not too uncommon occurance
//No breakpoints or watchpoints.


  // Set the new active flag so that after we have read the
  // breakpoint list we can trim the breakpoints that have been
  // removed (temporary breakpoints do this)
  activeFlag_++;

  // Try for a little flicker free
  setAutoUpdate(false);

  // skip the first line which is the header
  while (str && (str = strchr(str, '\n')))
  {
    str++;
    int id = atoi(str);
    if (id)
    {
      // Inner loop handles lines of extra data for this breakpoint
      // eg
      //  3   breakpoint     keep y   0x0804a847 in main at main.cpp:23
      //         breakpoint already hit 1 time"
      int hits = 0;
      int ignore = 0;
      QString condition;
      while (str && (str = strchr(str, '\n')))
      {
        str++;

        // The char after a newline is a digit hence it's
        // a new breakpt. Breakout to deal with this breakpoint.
        if (isdigit(*str))
        {
          str--;
          break;
        }

        // We're only interested in these fields here.
        if (strncmp(str, "\tbreakpoint already hit ", 24) == 0)
		      hits = atoi(str+24);
		
        if (strncmp(str, "\tignore next ", 13) == 0)
		      ignore = atoi(str+13);

        if (strncmp(str, "\tstop only if ", 14) == 0)
        {
          char* EOL = strchr(str, '\n');
          if (EOL)
		        condition = QString(str+14, EOL-(str+13));
		    }
      }

      if (Breakpoint* BP = findId(id))
			{
        BP->setActive(activeFlag_, id);
        BP->setHits(hits);
		    BP->setIgnoreCount(ignore);
		    BP->setConditional(condition);
        emit publishBPState(BP);

        BP->configureDisplay();
      }
    }
  }
  		
  // Remove any inactive breakpoints.
  for (int index=0; index<(int)count(); index++)
  {
    Breakpoint* BP = (Breakpoint*)(item(index));
    if (!BP->isActive(activeFlag_))
    {
      BP->setActionDie();
      emit publishBPState(BP);

      removeItem(index);
    }
  }

  setAutoUpdate(true);
  repaint();
}

/***************************************************************************/

void BreakpointManager::slotParseGDBBreakpointSet(char* str, int BPKey)
{
  char* startNo=0;
  bool hardware = false;
  Breakpoint* BP= findKey(BPKey);
  if (!BP)
    return;   // Why ?? Possibly internal dbgController BPs that shouldn't get here!

  BP->setDbgProcessing(false);

  if ((strncmp(str, "Breakpoint ", 11) == 0))
    startNo = str+11;
	else
	{
  	if ((strncmp(str, "Hardware watchpoint ", 20) == 0))
  	{
  	  hardware = true;
    	startNo = str+20;
    }
    else
  	  if ((strncmp(str, "Watchpoint ", 11) == 0))
    	  startNo = str+11;
  }
	
  if (startNo)
  {
    int id = atoi(startNo);
    if (id)
    {
      BP->setActive(activeFlag_, id);
      BP->setHardwareBP(hardware);
      emit publishBPState(BP);

      BP->configureDisplay();
      repaint();
    }
  }
}

/***************************************************************************/
