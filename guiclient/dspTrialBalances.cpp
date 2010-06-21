/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspTrialBalances.h"

#include <math.h>

#include <QMenu>
#include <QSqlError>
#include <QVariant>

#include <metasql.h>
#include <openreports.h>

#include "dspGLTransactions.h"
#include "storedProcErrorLookup.h"
#include "mqlutil.h"

dspTrialBalances::dspTrialBalances(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_trialbal, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*)), this, SLOT(sPopulateMenu(QMenu*,QTreeWidgetItem*)));

  _trialbal->addColumn(tr("Start"),       _dateColumn,     Qt::AlignCenter, true,  "period_start" );
  _trialbal->addColumn(tr("End"),         _dateColumn,     Qt::AlignCenter, true,  "period_end" );
  _trialbal->addColumn(tr("Account #"),   _itemColumn,     Qt::AlignCenter, true,  "account" );
  _trialbal->addColumn(tr("Description"), -1,              Qt::AlignLeft,   true,  "accnt_descrip"   );
  _trialbal->addColumn(tr("Beg. Bal."),   _bigMoneyColumn, Qt::AlignRight,  true,  "beginning"  );
  _trialbal->addColumn("",                25,              Qt::AlignLeft,   true,  "beginningsense"   );
  _trialbal->addColumn(tr("Debits"),      _bigMoneyColumn, Qt::AlignRight,  true,  "debits"  );
  _trialbal->addColumn(tr("Credits"),     _bigMoneyColumn, Qt::AlignRight,  true,  "credits"  );
  _trialbal->addColumn(tr("Difference"),  _bigMoneyColumn, Qt::AlignRight,  true,  "diff"  );
  _trialbal->addColumn("",                25,              Qt::AlignLeft,   true,  "diffsense"   );
  _trialbal->addColumn(tr("End Bal."),    _bigMoneyColumn, Qt::AlignRight,  true,  "ending"  );
  _trialbal->addColumn("",                25,              Qt::AlignLeft,   true,  "endingsense"   );
}

dspTrialBalances::~dspTrialBalances()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspTrialBalances::languageChange()
{
  retranslateUi(this);
}

void dspTrialBalances::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *)
{
  int menuItem;
  menuItem = pMenu->insertItem(tr("View Transactions..."), this, SLOT(sViewTransactions()), 0);

  if (_metrics->boolean("ManualForwardUpdate"))
  {
    pMenu->insertSeparator();
    menuItem = pMenu->insertItem(tr("Forward Update"), this, SLOT(sForwardUpdate()), 0);
  }
}

void dspTrialBalances::sViewTransactions()
{
  ParameterList params;
  params.append("accnt_id", _trialbal->id());
  params.append("period_id", _trialbal->altId());
  params.append("run");

  dspGLTransactions *newdlg = new dspGLTransactions();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void dspTrialBalances::sForwardUpdate()
{
  q.prepare( "SELECT MIN(forwardUpdateTrialBalance(trialbal_id)) AS result "
             "FROM trialbal "
             "WHERE ( (trialbal_period_id=:period_id)"
             " AND (trialbal_accnt_id=:accnt_id) );");
  q.bindValue(":accnt_id", _trialbal->id());
  q.bindValue(":period_id", _trialbal->altId());
  q.exec();
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("forwardUpdateTrialBalance", result), __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void dspTrialBalances::setParams(ParameterList & params)
{
  if (_selectedAccount->isChecked())
    params.append("accnt_id", _account->id());

  if (_selectedPeriod->isChecked())
    params.append("period_id", _period->id());
    
  if (_showZero->isChecked() || (_selectedAccount->isChecked() && _selectedPeriod->isChecked()))
    params.append("showZero");
}

void dspTrialBalances::sPrint()
{
  if (!_metrics->boolean("ManualForwardUpdate"))
  {
    if (!forwardUpdate())
      return;
  }
  
  ParameterList params;
  setParams(params);

  orReport report("TrialBalances", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspTrialBalances::sFillList()
{
  _trialbal->clear();
  if (!_metrics->boolean("ManualForwardUpdate"))
  {
    if (!forwardUpdate())
      return;
  }
  
  ParameterList params;
  setParams(params);
  MetaSQLQuery mql = mqlLoad("trialBalances", "detail");
  XSqlQuery r = mql.toQuery(params);
  if (r.first())
  {
    _trialbal->populate(r, true);
  }
  else if (r.lastError().type() != QSqlError::NoError)
  {
    systemError(this, r.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

bool dspTrialBalances::forwardUpdate()
{
  QString sql( "SELECT MIN(forwardUpdateAccount(accnt_id)) AS result "
               "FROM accnt "
               "<? if exists(\"accnt_id\") ?>"
               " WHERE (accnt_id=<? value(\"accnt_id\") ?>)"
               "<? endif ?>"
               ";" );

  ParameterList params;
  setParams(params);
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.first())
  {
    int result = q.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("forwardUpdateTrialBalance", result), __FILE__, __LINE__);
      return false;
    }
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return false;
  }
  return true;
}
