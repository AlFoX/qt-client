/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is xTuple ERP: PostBooks Edition 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by xTuple ERP: PostBooks Edition
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "honorific.h"

#include <QSqlError>
#include <QMessageBox>
#include <QValidator>
#include <QVariant>

/*
 *  Constructs a honorific as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
honorific::honorific(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
    connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_code, SIGNAL(lostFocus()), this, SLOT(sCheck()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
honorific::~honorific()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void honorific::languageChange()
{
    retranslateUi(this);
}

enum SetResponse honorific::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("hnfc_id", &valid);
  if (valid)
  {
    _honorificid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _code->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _code->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _code->setEnabled(FALSE);
      _save->hide();
      _close->setText(tr("&Close"));

      _close->setFocus();
    }
  }

  return NoError;
}

void honorific::sCheck()
{
  _code->setText(_code->text().trimmed());
  if ( (_mode == cNew) && (_code->text().length()) )
  {
    q.prepare( "SELECT hnfc_id "
               "FROM hnfc "
               "WHERE (UPPER(hnfc_code)=UPPER(:hnfc_code));" );
    q.bindValue(":hnfc_code", _code->text());
    q.exec();
    if (q.first())
    {
      _honorificid = q.value("hnfc_id").toInt();
      _mode = cEdit;
      populate();

      _code->setEnabled(FALSE);
    }
  }
}

void honorific::sSave()
{
  if (_code->text().length() == 0)
  {
      QMessageBox::warning( this, tr("Cannot Save Title"),
                            tr("You must enter a valid Title.") );
      return;
  }
  
  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('hnfc_hnfc_id_seq') AS _hnfc_id");
    if (q.first())
      _honorificid = q.value("_hnfc_id").toInt();
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "INSERT INTO hnfc "
               "( hnfc_id, hnfc_code ) "
               "VALUES "
               "( :hnfc_id, :hnfc_code );" );
  }
  else if (_mode == cEdit)
  {
    q.prepare( "SELECT hnfc_id "
               "FROM hnfc "
               "WHERE ( (UPPER(hnfc_code)=UPPER(:hnfc_code))"
               " AND (hnfc_id<>:hnfc_id) );" );
    q.bindValue(":hnfc_id", _honorificid);
    q.bindValue(":hnfc_code", _code->text());
    q.exec();
    if (q.first())
    {
      QMessageBox::warning( this, tr("Cannot Save Title"),
                            tr("You may not rename this Title with the entered value as it is in use by another Title.") );
      return;
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    q.prepare( "UPDATE hnfc "
               "SET hnfc_code=:hnfc_code "
               "WHERE (hnfc_id=:hnfc_id);" );
  }

  q.bindValue(":hnfc_id", _honorificid);
  q.bindValue(":hnfc_code", _code->text());
  q.exec();
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_honorificid);
}

void honorific::populate()
{
  q.prepare( "SELECT hnfc_code "
             "FROM hnfc "
             "WHERE (hnfc_id=:hnfc_id);" );
  q.bindValue(":hnfc_id", _honorificid);
  q.exec();
  if (q.first())
  {
    _code->setText(q.value("hnfc_code").toString());
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
