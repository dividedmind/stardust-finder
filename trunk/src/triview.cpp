/***************************************************************************
 *   Copyright (C) 2007 by Rafał Rzepecki   *
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
#include "triview.h"
#include "ui_triview.h"
#include "standardanalyzer.h"

#include <QImage>
#include <QPainter>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

TriView::TriView( QWidget *parent )
    : QWidget( parent ), m_ui( new Ui_TriView )
{
  m_ui->setupUi( this );
  setCrosshairsShown(false);
  
  connect(m_ui->xSlicer, SIGNAL(valueChanged(int)), 
          m_ui->sideView, SLOT(showSlice(int)));
  connect(m_ui->xSlicer, SIGNAL(valueChanged(int)), 
          m_ui->topView, SLOT(xMove(int)));
  connect(m_ui->xSlicer, SIGNAL(valueChanged(int)), 
          m_ui->frontView, SLOT(xMove(int)));
  connect(m_ui->ySlicer, SIGNAL(valueChanged(int)), 
          m_ui->frontView, SLOT(showSlice(int)));
  connect(m_ui->ySlicer, SIGNAL(valueChanged(int)), 
          m_ui->topView, SLOT(yMove(int)));
  connect(m_ui->ySlicer, SIGNAL(valueChanged(int)), 
          m_ui->sideView, SLOT(yMove(int)));
  connect(m_ui->zSlicer, SIGNAL(valueChanged(int)), 
          m_ui->topView, SLOT(showSlice(int)));
  connect(m_ui->zSlicer, SIGNAL(valueChanged(int)), 
          m_ui->frontView, SLOT(yMove(int)));
  connect(m_ui->zSlicer, SIGNAL(valueChanged(int)), 
          m_ui->sideView, SLOT(xMove(int)));
  
  connect(m_ui->topView, SIGNAL(xMoved(int)),
          m_ui->xSlicer, SLOT(setValue(int)));
  connect(m_ui->topView, SIGNAL(yMoved(int)),
          m_ui->ySlicer, SLOT(setValue(int)));
  connect(m_ui->sideView, SIGNAL(xMoved(int)),
          m_ui->zSlicer, SLOT(setValue(int)));
  connect(m_ui->sideView, SIGNAL(yMoved(int)),
          m_ui->ySlicer, SLOT(setValue(int)));
  connect(m_ui->frontView, SIGNAL(xMoved(int)),
          m_ui->xSlicer, SLOT(setValue(int)));
  connect(m_ui->frontView, SIGNAL(yMoved(int)),
          m_ui->zSlicer, SLOT(setValue(int)));
  
  connect(m_ui->xSlicer, SIGNAL(valueChanged(int)),
          SIGNAL(moved()));
  connect(m_ui->ySlicer, SIGNAL(valueChanged(int)),
          SIGNAL(moved()));
  connect(m_ui->zSlicer, SIGNAL(valueChanged(int)),
          SIGNAL(moved()));
  
  analyzer = new StandardAnalyzer();
  qRegisterMetaType<TriView::ProcessedImages>("TriView::ProcessedImages");
  connect(analyzer, SIGNAL(finished(const TriView::ProcessedImages&)), 
          SLOT(analysisFinished(const TriView::ProcessedImages &)));
}

void TriView::setImages(const QVector<QImage> &images)
{
  m_ui->topView->clear();
  m_ui->sideView->clear();
  m_ui->frontView->clear();
  
  analyzer->analyze(images.toList());
}

void TriView::analysisFinished(const ProcessedImages &images)
{
  m_ui->topView->setImages(images.top);
  m_ui->sideView->setImages(images.side);
  m_ui->frontView->setImages(images.front);
  
  m_ui->zSlicer->setRange(0, images.top.size()-1);
  m_ui->zSlicer->setValue(0);
  m_ui->zSlicer2->setRange(0, images.top.size()-1);
  m_ui->zSlicer2->setValue(0);
  m_ui->xSlicer->setRange(0, images.side.size()-1);
  m_ui->xSlicer->setValue(0);
  m_ui->ySlicer->setRange(0, images.front.size()-1);
  m_ui->ySlicer->setValue(0);
  
  emit imageLoaded();
}

void TriView::insertExtraWidget( QWidget *widget )
{
  m_ui->gridLayout->addWidget(widget, 2, 2, 2, 2);
}

int TriView::getX() const
{
  return m_ui->xSlicer->value();
}

int TriView::getY() const
{
  return m_ui->ySlicer->value();
}

int TriView::getZ() const
{
  return m_ui->zSlicer->value();
}

TriView::~TriView()
{}


/*!
    \fn TriView::clear
 */
void TriView::clear()
{
  m_ui->topView->clear();
  m_ui->sideView->clear();
  m_ui->frontView->clear();

  m_ui->zSlicer->setRange(0, 0);
  m_ui->zSlicer2->setRange(0, 0);
  m_ui->xSlicer->setRange(0, 0);
  m_ui->ySlicer->setRange(0, 0);
}


bool TriView::crosshairsShown() const
{
    return m_crosshairsShown;
}


void TriView::setCrosshairsShown ( bool theValue )
{
  m_ui->topView->setDrawingLines(theValue);
  m_ui->sideView->setDrawingLines(theValue);
  m_ui->frontView->setDrawingLines(theValue);
  m_crosshairsShown = theValue;
  update();
}
