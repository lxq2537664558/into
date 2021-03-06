/* This file is part of Into.
 * Copyright (C) Intopii 2013.
 * All rights reserved.
 *
 * Licensees holding a commercial Into license may use this file in
 * accordance with the commercial license agreement. Please see
 * LICENSE.commercial for commercial licensing terms.
 *
 * Alternatively, this file may be used under the terms of the GNU
 * Affero General Public License version 3 as published by the Free
 * Software Foundation. In addition, Intopii gives you special rights
 * to use Into as a part of open source software projects. Please
 * refer to LICENSE.AGPL3 for details.
 */

#include "PiiResourceDatabase.h"

namespace Pii
{
  Subject subject;
  Predicate predicate;
  Object object;
  Attribute attribute("");
  ResourceType resourceType;
  StatementId statementId;
}

PiiResourceDatabase::PiiResourceDatabase() :
  d(new Data)
{}

PiiResourceDatabase::~PiiResourceDatabase()
{
  delete d;
}

PiiResourceStatement PiiResourceDatabase::resource(const char* subject, const char* predicate, const char* object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Resource);
}

PiiResourceStatement PiiResourceDatabase::literal(const char* subject, const char* predicate, const char* object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Literal);
}

PiiResourceStatement PiiResourceDatabase::resource(const QString& subject, const QString& predicate, const QString& object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Resource);
}

PiiResourceStatement PiiResourceDatabase::literal(const QString& subject, const QString& predicate, const QString& object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Literal);
}

PiiResourceStatement PiiResourceDatabase::resource(const char* subject, const char* predicate, void* object)
{
  return PiiResourceStatement(subject, predicate, object);
}

PiiResourceStatement PiiResourceDatabase::resource(const QString& subject, const QString& predicate, void* object)
{
  return PiiResourceStatement(subject, predicate, object);
}

PiiResourceStatement PiiResourceDatabase::resource(int subject, const QString& predicate, const QString& object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Resource);
}

PiiResourceStatement PiiResourceDatabase::literal(int subject, const QString& predicate, const QString& object)
{
  return PiiResourceStatement(subject, predicate, object, PiiResourceStatement::Literal);
}

int PiiResourceDatabase::generateId()
{
  // PENDING overflow handling
  return d->lstStatements.size() > 0 ? d->lstStatements.back().id() + 1 : 0;
}

int PiiResourceDatabase::addStatement(const PiiResourceStatement& statement)
{
  PiiResourceStatement copy(statement);
  int id = generateId();
  copy.setId(id);
  d->lstStatements.push_back(copy);
  return id;
}

QList<int> PiiResourceDatabase::addStatements(const QList<PiiResourceStatement>& statements)
{
  QList<int> lstIds;
  int id = -1;
  for (int i=0; i<statements.size(); ++i)
    {
      const PiiResourceStatement& s = statements[i];
      // "#" refers to the id of the previous statement (that is not a
      // reification of another statement)
      if (s.subject() == "#" && i != 0)
        {
          if (s.type() != PiiResourceStatement::Pointer)
            lstIds.push_back(addStatement(PiiResourceStatement(QString("#%1").arg(id), s.predicate(), s.object(), s.type())));
          else
            lstIds.push_back(addStatement(PiiResourceStatement(id, s.predicate(), s.object(), s.type())));
        }
      else
        {
          id = addStatement(s);
          lstIds.push_back(id);
        }
    }
  return lstIds;
}

void PiiResourceDatabase::removeStatements(const QList<int>& ids)
{
  for (int i=0; i<ids.size(); ++i)
    removeStatement(ids[i]);
}

void PiiResourceDatabase::removeStatement(int id)
{
  for (int i=0; i<d->lstStatements.size(); ++i)
    {
      if (d->lstStatements[i].id() == id)
        {
          d->lstStatements.removeAt(i);
          return;
        }
    }
}

QList<PiiResourceStatement> PiiResourceDatabase::statements() const
{
  return d->lstStatements;
}

int PiiResourceDatabase::statementCount() const
{
  return d->lstStatements.size();
}

void PiiResourceDatabase::dump() const
{
  for (int i=0; i<d->lstStatements.size(); ++i)
    {
      QString strStatement("(%1, %2, %4) #%3");
      strStatement = strStatement
        .arg(d->lstStatements[i].subject())
        .arg(d->lstStatements[i].predicate())
        .arg(d->lstStatements[i].id());
      if (d->lstStatements[i].type() == PiiResourceStatement::Literal)
        strStatement = strStatement.arg(QString("\"%1\"").arg(d->lstStatements[i].object()));
      else
        strStatement = strStatement.arg(d->lstStatements[i].object());
      piiWarning("%s", piiPrintable(strStatement));
    }
}
