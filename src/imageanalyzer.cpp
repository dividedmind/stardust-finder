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
#include "imageanalyzer.h"

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

ImageAnalyzer::ImageAnalyzer(QObject *parent)
  : QThread(parent)
{
  mutex = new QMutex();
  condition = new QWaitCondition();
  m_restart = m_abort = false;
}

ImageAnalyzer::~ImageAnalyzer()
{
  m_abort = true;
  condition->wakeOne();
  wait();
}

void ImageAnalyzer::analyze(const QList<QImage> &images)
{
  foreach (QImage image, images)
    if (!image.isGrayscale()) {
      qWarning("Images are not grayscale.");
      return;
    }
  QMutexLocker locker(mutex);
  m_images = images;
  
  if (!isRunning())
    start(LowPriority)
    ;
  else {
    m_restart = true;
    condition->wakeOne();
  }
}

bool ImageAnalyzer::abort() const
{
  return m_abort || m_restart;
}

void ImageAnalyzer::run()
{
  for(;;) {
    mutex->lock();
    QList<QImage> images = m_images;
    m_images.clear();
    mutex->unlock();
    TriView::ProcessedImages *result = performAnalysis(images);
    images.clear();
    mutex->lock();
    if (result && !abort())
      emit finished(*result);
    if (result)
      delete result;
    if (m_abort) {
      mutex->unlock();
      return;
    }
    if (!m_restart)
      condition->wait(mutex);
    m_restart = false;
    mutex->unlock();
  }
}
