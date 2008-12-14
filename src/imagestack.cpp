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
#include "imagestack.h"

#include <QImage>
#include <QVector>
#include <QPoint>
#include <QMouseEvent>
#include <QPainter>

ImageStack::ImageStack( QWidget *parent )
    : QLabel( parent ), m_drawingLines(true)
{
  m_painter = new QPainter();
}

void ImageStack::addImage( const QImage &image )
{
  if (m_stack.isEmpty())
    setPixmap(QPixmap::fromImage(image));
  m_stack.append(image);
}

void ImageStack::clear()
{
  m_stack.clear();
  m_slice = 0;
}

ImageStack::~ImageStack()
{}

void ImageStack::mousePressEvent(QMouseEvent * event)
{
  checkMovement(event->pos());
}

void ImageStack::mouseMoveEvent(QMouseEvent * event)
{
  if (event->buttons() & Qt::LeftButton)
    checkMovement(event->pos());
}

void ImageStack::paintEvent(QPaintEvent * event)
{
  QLabel::paintEvent(event);
  if (m_drawingLines) {
    m_painter->begin(this);
    m_painter->drawLine(m_pos.x(), 0, m_pos.x(), height());
    m_painter->drawLine(0, m_pos.y(), width(), m_pos.y());
    m_painter->end();
  }
}

void ImageStack::checkMovement(QPoint pos)
{
  pos.setX(qMax(0, qMin(pos.x(), width()-1)));
  pos.setY(qMax(0, qMin(pos.y(), height()-1)));
  if (m_pos.x() != pos.x())
    emit xMoved(pos.x());
  if (m_pos.y() != pos.y())
    emit yMoved(pos.y());
  
  if (m_pos != pos)
    showSlice(m_slice);
  
  m_pos = pos;
}

void ImageStack::xMove(int newX)
{
  QPoint new_pos = m_pos;
  new_pos.setX(newX);
  checkMovement(new_pos);
}

void ImageStack::yMove(int newY)
{
  QPoint new_pos = m_pos;
  new_pos.setY(newY);
  checkMovement(new_pos);
}

/*!
    \fn ImageStack::showSlice(int slice)
 */
void ImageStack::showSlice(int slice)
{
  if (0 <= slice && slice < m_stack.size()) {
    m_slice = slice;
    setPixmap(QPixmap::fromImage(m_stack[slice]));
  }
}

void ImageStack::setDrawingLines(bool drawingLines)
{
  m_drawingLines = drawingLines;
}

bool ImageStack::isDrawingLines() const
{
  return m_drawingLines;
}
