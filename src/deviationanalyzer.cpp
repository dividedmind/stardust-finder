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
#include "deviationanalyzer.h"

#include <cmath>

DeviationAnalyzer::DeviationAnalyzer(QObject *parent)
 : ImageAnalyzer(parent)
{
}


DeviationAnalyzer::~DeviationAnalyzer()
{
}

TriView::ProcessedImages *DeviationAnalyzer::performAnalysis(QList<QImage> top) const
{
  int width = top[0].width();
  int height = top[0].height();
  int depth = top.size();
  QImage::Format format = top[0].format();
  QVector<QRgb> colortable(256);
  for (int i = 0; i < 256; ++i)
    colortable[i] = rgbFromWaveLength(380.0 + (i * 400.0 / 256));
  
  QList<QImage> side;
  QList<QImage> front;
  
  QImage average(width, height, format);
  int maxDev = 0;
  average.setColorTable(colortable);
  for (int x = 0; x < width; x++)
    for (int y = 0; y < height; y++) {
      float mean = 0.0;
      for (int z = 0; z < depth; z++)
        mean += top[z].pixelIndex(x, y);
      mean /= depth;
      for (int z = 0; z < depth; z++)
        maxDev = qMax(maxDev, abs(top[z].pixelIndex(x, y) - (int) mean));
      average.setPixel(x, y, mean);
    }

  for (int z = 0; z < depth; z++) {
    top[z].setColorTable(colortable);
    for (int x = 0; x < width; x++)
      for (int y = 0; y < height; y++) {
        int index = top[z].pixelIndex(x, y) - average.pixelIndex(x, y);
        index = 127*index/maxDev + 127;
        top[z].setPixel(x, y, index);
      }
  }
    
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

uint DeviationAnalyzer::rgbFromWaveLength(double wave)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if (wave >= 380.0 && wave <= 440.0) {
        r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
        b = 1.0;
    } else if (wave >= 440.0 && wave <= 490.0) {
        g = (wave - 440.0) / (490.0 - 440.0);
        b = 1.0;
    } else if (wave >= 490.0 && wave <= 510.0) {
        g = 1.0;
        b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
    } else if (wave >= 510.0 && wave <= 580.0) {
        r = (wave - 510.0) / (580.0 - 510.0);
        g = 1.0;
    } else if (wave >= 580.0 && wave <= 645.0) {
        r = 1.0;
        g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
    } else if (wave >= 645.0 && wave <= 780.0) {
        r = 1.0;
    }

    double s = 1.0;
    if (wave > 700.0)
        s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
    else if (wave <  420.0)
        s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);

    r = pow(r * s, 0.8);
    g = pow(g * s, 0.8);
    b = pow(b * s, 0.8);
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}
