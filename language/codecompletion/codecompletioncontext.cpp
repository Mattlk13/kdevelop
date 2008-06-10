/*
   Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "codecompletioncontext.h"

#include "pushvalue.h"

using namespace KDevelop;

typedef PushValue<int> IntPusher;

///Extracts the last line from the given string
QString CodeCompletionContext::extractLastLine(const QString& str) {
  int prevLineEnd = str.lastIndexOf('\n');
  if(prevLineEnd != -1)
    return str.mid(prevLineEnd+1);
  else
    return str;
}

int completionRecursionDepth = 0;

CodeCompletionContext::CodeCompletionContext(DUContextPointer context, const QString& text, int depth, const QStringList& knownArgumentExpressions, int line ) : m_text(text), m_depth(depth), m_duContext(context), m_parentContext(0)
{
  IntPusher( completionRecursionDepth, completionRecursionDepth+1 );

  if( depth > 10 ) {
    log( "CodeCompletionContext::CodeCompletionContext: too much recursion" );
    m_valid = false;
    return;
  }
  
  if( completionRecursionDepth > 10 ) {
    log( "CodeCompletionContext::CodeCompletionContext: too much recursion" );
    m_valid = false;
    return;
  }
}

CodeCompletionContext::~CodeCompletionContext() {
}

int CodeCompletionContext::depth() const {
  return m_depth;
}

void CodeCompletionContext::log( const QString& str ) const {
  kDebug() << "CodeCompletionContext:" << str;
}
