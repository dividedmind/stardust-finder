/***************************************************************************
 *   Copyright (C) 2007 by Rafa? Rzepecki   *
 *   divided.mind@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include "ui_aboutbox.h"

class QTextEdit;

class AboutBox : public QDialog, private Ui::AboutBox
{
  Q_OBJECT
  Q_PROPERTY(QString version READ version WRITE setVersion)

public:
  AboutBox(QWidget* parent = 0, Qt::WFlags fl = 0 );
  AboutBox(const QString &version, QWidget* parent = 0, Qt::WFlags fl = 0 );
  ~AboutBox();

    void setVersion ( const QString& theValue );
    

    QString version() const;
    
  /*$PUBLIC_FUNCTIONS$*/

public slots:
  /*$PUBLIC_SLOTS$*/

protected:
  /*$PROTECTED_FUNCTIONS$*/

private:
    QString m_version;
    QString aboutText;
};

#endif

