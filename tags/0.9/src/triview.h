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
#ifndef TRIVIEW_H
#define TRIVIEW_H

#include <QWidget>
#include <QVector>
#include <QThread>

class Ui_TriView;
class QMutex;
class QWaitCondition;
class ImageAnalyzer;

/**
 @author Rafał Rzepecki <divided.mind@gmail.com>
*/
class TriView : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int X READ getX)
  Q_PROPERTY(int Y READ getY)
  Q_PROPERTY(int Z READ getZ)

public:
  TriView( QWidget *parent = 0 );
  void setImages( const QVector<QImage> &images );
  void insertExtraWidget( QWidget *widget );
  int getX() const;
  int getY() const;
  int getZ() const;

  ~TriView();
  void clear();

    

    bool crosshairsShown() const;
    
  
  typedef struct {
    QList<QImage> top, front, side;
  } ProcessedImages;
  
signals:
  void moved();
  void imageLoaded();

private:
  Ui_TriView *m_ui;
  ImageAnalyzer *analyzer;
    bool m_crosshairsShown;
  
private slots:
  void setCrosshairsShown ( bool theValue );
  void analysisFinished(const TriView::ProcessedImages &images);
};

#endif
