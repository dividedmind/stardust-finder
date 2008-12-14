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
#include "standardanalyzer.h"
#include "triview.h"

#include <QList>
#include <QImage>
#include <QPainter>

#include <string.h>

StandardAnalyzer::StandardAnalyzer(QObject *parent)
 : ImageAnalyzer(parent)
{
}


StandardAnalyzer::~StandardAnalyzer()
{
}

TriView::ProcessedImages *StandardAnalyzer::performAnalysis(QList<QImage> top) const
{
  int width = top[0].width();
  int height = top[0].height();
  int depth = top.size();
  QImage::Format format = top[0].format();
  QVector<QRgb> colortable = top[0].colorTable();
  
  QList<QImage> side;
  QList<QImage> front;

  // construct side view images
  for (int x = 0; x < width; x++) {
    if (abort())
      return 0;
    QImage image(depth, height, format);
    image.setColorTable(colortable);
    for (int z = 0; z < depth; z++)
      for (int y = 0; y < height; y++)
        image.setPixel(z, y, top[z].pixelIndex(x, y));
    side.append(image);
  }
  
  // construct front view images
  for (int y = 0; y < height; y++) {
    if (abort())
      return 0;
    QImage image(width, depth, format);
    image.setColorTable(colortable);
    for (int z = 0; z < depth; z++)
      memcpy(image.scanLine(z), top[z].scanLine(y), top[z].bytesPerLine());
    front.append(image);
  }
  
  TriView::ProcessedImages result = { top, front, side };
  return new TriView::ProcessedImages(result);
}
