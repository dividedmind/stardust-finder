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
#ifndef IMAGEANALYZER_H
#define IMAGEANALYZER_H

#include "triview.h"
#include <QThread>
#include <QList>
#include <QImage>

class QMutex;
class QWaitCondition;

/**
	@author Rafał Rzepecki <divided.mind@gmail.com>
*/

class ImageAnalyzer : public QThread
{
  Q_OBJECT
      
  public:
    ImageAnalyzer(QObject *parent = 0);
    ~ImageAnalyzer();
    void analyze(const QList<QImage> &images);
  
  protected:
    void run();
    bool abort() const;
    virtual TriView::ProcessedImages *performAnalysis(QList<QImage> top) const = 0;
  signals:
    void finished(const TriView::ProcessedImages &images);
    
  private:
    QMutex *mutex;
    QWaitCondition *condition;
    QList<QImage> m_images;
    bool m_abort, m_restart;
};


#endif
