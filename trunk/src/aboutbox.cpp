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


#include "aboutbox.h"
#include <QPushButton>
#include <QTextEdit>
#include <QFile>
#include <QtDebug>

AboutBox::AboutBox(QWidget* parent, Qt::WFlags fl)
: QDialog( parent, fl ), Ui::AboutBox()
{
	setupUi(this);
    aboutText = aboutLabel->text();
    QFile license(":/COPYING");
    license.open(QFile::ReadOnly);
    licenseViewer->setPlainText(license.readAll());
}

AboutBox::AboutBox(const QString &version, QWidget* parent, Qt::WFlags fl)
  : QDialog( parent, fl), Ui::AboutBox()
{
  setupUi(this);
  aboutText = aboutLabel->text();
  QFile license(":/COPYING");
  license.open(QFile::ReadOnly);
  licenseViewer->setPlainText(license.readAll());
  setVersion(version);
}

AboutBox::~AboutBox()
{
}

/*$SPECIALIZATION$*/


QString AboutBox::version() const
{
    return m_version;
}


void AboutBox::setVersion ( const QString& theValue )
{
    m_version = theValue;
    aboutLabel->setText(aboutText.arg(m_version));
}
