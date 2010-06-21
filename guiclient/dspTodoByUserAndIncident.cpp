/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspTodoByUserAndIncident.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <parameter.h>
#include <openreports.h>
#include <metasql.h>

#include "incident.h"
#include "todoItem.h"
#include "mqlutil.h"

dspTodoByUserAndIncident::dspTodoByUserAndIncident(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);
//  statusBar();

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_query, SIGNAL(clicked()), this, SLOT(sFillList()));
  connect(_todoitem, SIGNAL(populateMenu(QMenu*, QTreeWidgetItem*, int)), this, SLOT(sPopulateMenu(QMenu*)));

  _usr->setType(ParameterGroup::User);

  _dueDate->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _dueDate->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);
  _startDate->setStartNull(tr("Earliest"), omfgThis->startOfTime(), TRUE);
  _startDate->setEndNull(tr("Latest"), omfgThis->endOfTime(), TRUE);

  _todoitem->addColumn(tr("Assigned To"),  _userColumn, Qt::AlignCenter,true, "todoitem_username");
  _todoitem->addColumn(tr("Priority"),    _prcntColumn, Qt::AlignCenter,true, "incdtpriority_name");
  _todoitem->addColumn(tr("Incident"),    _orderColumn, Qt::AlignLeft,  true, "incdt_id", "incdt_number");
  _todoitem->addColumn(tr("Task Name"),            100, Qt::AlignLeft,  true, "todoitem_name");
  _todoitem->addColumn(tr("Status"),	 _statusColumn, Qt::AlignCenter,true, "todoitem_status");
  _todoitem->addColumn(tr("Date Due"),     _dateColumn, Qt::AlignCenter,true, "todoitem_due_date");
  _todoitem->addColumn(tr("Date Started"), _dateColumn, Qt::AlignCenter,true, "todoitem_start_date");
  _todoitem->addColumn(tr("Description"),           -1, Qt::AlignLeft,  true, "todoitem_description");

  _incident->setEnabled(_selectedIncident->isChecked());
}

dspTodoByUserAndIncident::~dspTodoByUserAndIncident()
{
  // no need to delete child widgets, Qt does it all for us
}

void dspTodoByUserAndIncident::languageChange()
{
  retranslateUi(this);
}

void dspTodoByUserAndIncident::sPopulateMenu(QMenu *pMenu)
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEditTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, _privileges->check("MaintainOtherTodoLists"));

  menuItem = pMenu->insertItem(tr("View..."), this, SLOT(sViewTodoItem()), 0);
  pMenu->setItemEnabled(menuItem, _privileges->check("ViewOtherTodoLists"));

  if (_todoitem->altId() > 0)
  {
    pMenu->insertSeparator();
    menuItem = pMenu->insertItem(tr("Edit Incident"), this, SLOT(sEditIncident()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("MaintainIncidents"));
    menuItem = pMenu->insertItem(tr("View Incident"), this, SLOT(sViewIncident()), 0);
    pMenu->setItemEnabled(menuItem, _privileges->check("ViewIncidents") ||
				    _privileges->check("MaintainIncidents"));
  }
}

void dspTodoByUserAndIncident::setParams(ParameterList& params)
{
  _usr->appendValue(params);
  if (_selectedIncident->isChecked())
    params.append("incdt_id", _incident->id());
  if (_showInactive->isChecked())
    params.append("showInactive");
  if (_showCompleted->isChecked())
    params.append("showCompleted");
  if (_startDate->startDate() > omfgThis->startOfTime())
    params.append("start_date_start", _startDate->startDate());
  if (_startDate->endDate() < omfgThis->endOfTime())
    params.append("start_date_end",   _startDate->endDate());
  if (_dueDate->startDate() > omfgThis->startOfTime())
    params.append("due_date_start",   _dueDate->startDate());
  if (_dueDate->endDate() < omfgThis->endOfTime())
    params.append("due_date_end",     _dueDate->endDate());
}

void dspTodoByUserAndIncident::sFillList()
{
  if (_selectedIncident->isChecked() && _incident->id() <= 0)
  {
    QMessageBox::critical(this, tr("No Incident"),
			  tr("Please select an Incident before Querying."),
			  QMessageBox::Ok, QMessageBox::NoButton);
    _incident->setFocus();
    return;
  }

  _todoitem->clear();

  ParameterList params;
  setParams(params);

  MetaSQLQuery mql = mqlLoad("todoByUserAndIncident", "detail");
  XSqlQuery todos = mql.toQuery(params);
  _todoitem->populate(todos);
  if (todos.lastError().type() != QSqlError::NoError)
  {
    systemError(this, todos.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void dspTodoByUserAndIncident::sPrint()
{
  ParameterList params;
  setParams(params);

  orReport report("TodoByUserAndIncident", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void dspTodoByUserAndIncident::sEditIncident()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("incdt_id", _todoitem->altId());

  incident newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspTodoByUserAndIncident::sEditTodoItem()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("todoitem_id", _todoitem->id());

  todoItem newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void dspTodoByUserAndIncident::sViewIncident()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("incdt_id", _todoitem->altId());

  incident newdlg(this, "", TRUE);
  newdlg.set(params);

  newdlg.exec();
}

void dspTodoByUserAndIncident::sViewTodoItem()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("todoitem_id", _todoitem->id());

  todoItem newdlg(this, "", TRUE);
  newdlg.set(params);

  newdlg.exec();
}
