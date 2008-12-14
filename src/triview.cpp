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

#include <QImage>
#include <QPainter>

TriView::TriView( QWidget *parent )
    : QWidget( parent ), m_ui( new Ui_TriView )
{
  m_ui->setupUi( this );
  
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
}

void TriView::setImages(const QVector<QImage> &images)
{
  m_ui->topView->clear();
  m_ui->sideView->clear();
  m_ui->frontView->clear();
  
  for (QVector<QImage>::const_iterator it = images.constBegin();
       it != images.constEnd(); ++it)
    m_ui->topView->addImage(*it);
  
  m_ui->zSlicer->setRange(0, images.size()-1);
  m_ui->zSlicer2->setRange(0, images.size()-1);
  m_ui->xSlicer->setRange(0, images[0].width()-1);
  m_ui->ySlicer->setRange(0, images[0].height()-1);
  
  deriveImages(images);
}

void TriView::deriveImages(const QVector<QImage> &images)
{
  int width = images[0].width();
  int height = images[0].height();
  int depth = images.size();
  QImage::Format format = images[0].format();
  format = QImage::Format_RGB16;
  
  // construct side view images
  for (int x = 0; x < width; x++) {
    QImage image(depth, height, format);
    QPainter p(&image);
    for (int z = 0; z < depth; z++)
      p.drawImage(z, 0, images[z], x, 0, 1);
    m_ui->sideView->addImage(image);
  }
  
  // construct front view images
  for (int y = 0; y < height; y++) {
    QImage image(width, depth, format);
    QPainter p(&image);
    for (int z = 0; z < depth; z++)
      p.drawImage(0, z, images[z], 0, y, -1, 1);
    m_ui->frontView->addImage(image);
  }
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
