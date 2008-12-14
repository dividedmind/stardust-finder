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
#ifndef IMAGESTACK_H
#define IMAGESTACK_H

#include <QLabel>
#include <QVector>
#include <QImage>
#include <QPoint>

class QPainter;

/**
 @author Rafał Rzepecki <divided.mind@gmail.com>
*/
class ImageStack : public QLabel
{
  Q_OBJECT
  Q_PROPERTY(bool DrawingLines READ isDrawingLines WRITE setDrawingLines)
public:
  ImageStack( QWidget *parent = 0 );

  void addImage( const QImage &image );
  void clear();
  bool isDrawingLines() const;

  ~ImageStack();

public slots:
  void showSlice(int slice);
  void xMove(int newX);
  void yMove(int newY);
  void setDrawingLines(bool drawLines);

signals:
  void xMoved(int newX);
  void yMoved(int newY);

protected:
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void paintEvent ( QPaintEvent * event );

private:
  void checkMovement(QPoint pos);
  QVector<QImage> m_stack;
  QPoint m_pos;
  int m_slice;
  bool m_drawingLines;
  QPainter *m_painter;
};

#endif
