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
#include "stardustconnector.h"

#include <QUrl>
#include <QByteArray>
#include <QByteArrayMatcher>
#include <QBuffer>
#include <QMessageBox>

StardustConnector::StardustConnector(QObject *parent)
 : QHttp(parent), host("stardustathome.ssl.berkeley.edu"), 
         currentState(NotConnected), currentId(0)
{
  setHost(host);
  connect(this, SIGNAL(requestFinished(int, bool)), SLOT(requestFinished(int, bool)));
  connect(this, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), 
          SLOT(responseHeaderReceived(const QHttpResponseHeader &)));
}

void StardustConnector::login(const QString &username, const QString &password)
{
  emit connecting();
  
  QMap<QString, QString> postParams;
  postParams["name"] = username;
  postParams["password"] = password;
  postParams["submit"] = "Submit";
  currentId = post("/ss_login_action.php", postParams);
  currentState = Connecting;
}

void StardustConnector::requestFinished(int id, bool err)
{
  if (id != currentId)
    return;
  currentId = 0;
  if (err) {
    emitError();
    return;
  }
  
  if (currentState == Connecting) {
    currentState = Connected;
    emit connected();
    requestImages();
  } else if (currentState == Connected) {
    parseMicroscope(readAll());
  }
}

void StardustConnector::respondTrack(int x, int y, int z)
{
  QString path = respPath.arg(x).arg(y).arg(z+1);
  requestImages(path);
}

void StardustConnector::respondNoFocus()
{
  respondTrack(-2, -2, -3);
}

void StardustConnector::respondNoTrack()
{
  respondTrack(-1, -1, -2);
}

void StardustConnector::requestImages(const QString &path)
{
  if (currentState != Connected)
    return;
  
  currentId = auth_get(path);
}

void StardustConnector::emitError()
{
  emit error();
  if (currentState == Connecting)
    currentState = NotConnected;
}

void StardustConnector::parseMicroscope(const QByteArray &page)
{
  static const QByteArrayMatcher bam_frames("var next_movie_frames");
  static const QByteArrayMatcher bam_next_frames("var ondeck_movie_frames");
  static const QByteArrayMatcher bam_filename("full_filename(");
  static const QByteArrayMatcher bam_path("ss_virtual_microscope.php?");
  static const QByteArrayMatcher bam_info("Calibration Movies");
  static const QByteArrayMatcher bam_end_info("</table>");
  
  int frames;
  int i = bam_frames.indexIn(page) + 30;
  frames = page.mid(i, page.indexOf(')', i) - i).toInt();
  
  int next_frames;
  i = bam_next_frames.indexIn(page, i) + 32;
  next_frames = page.mid(i, page.indexOf(')', i) - i).toInt();
  
  QString filename;
  i = bam_filename.indexIn(page, i) + 15;
  filename = page.mid(i, page.indexOf('"', i) - i);
  
  QString next_filename;
  i = bam_filename.indexIn(page, i) + 15;
  next_filename = page.mid(i, page.indexOf('"', i) - i);
  
  QString path;
  i = bam_path.indexIn(page, i);
  path = page.mid(i, page.indexOf('\'', i) - i);
  
  QString info_part;
  i = bam_info.indexIn(page, i);
  info_part = page.mid(i, bam_end_info.indexIn(page, i) - i);
  
  emit newImage(QUrl(filename), frames);
  emit nextImage(QUrl(next_filename), frames);
  respPath = QString("/") + path + "%3&coords=?%1,%2";
  parseInfo(info_part);
}

void StardustConnector::parseInfo(const QString &info)
{
  QString niceInfo = tr("<table style=\"th { text-align: right; }\">\
      <tr><th>Movie id:</th><td>%2</td></tr>\
      <tr><th>Correct calibrations:</th><td>%1</td></tr>\
      <tr><th>Incorrect calibrations:</th><td>%3</td></tr>\
      <tr><th>Overall score:</th><td>%4</td></tr>\
      <tr><th>Real movies:</th><td>%5</td></tr>\
      <tr><th>Rank:</th><td>%6/%7</td></tr>\
      <tr><th>Specifity:</th><td>%8%</td></tr>\
      <tr><th>Sensitivity:</th><td>%9%</td></tr></table>");
  QRegExp rx("Answered Correctly</b></td>\\s*<td>(\\d+)</td>.*Movie id:</b> </td>\\s*<td>\\s*(\\S*).*Calibration Movies</a> Answered Incorrectly</b></td>\\s*<td>(\\d+).*Your Overall Score</a></b>:</td><td>\\s*(-?\\d+).*Real Movies</a> Viewed</b>:</td><td>\\s*(\\d+).*Your Rank</a></b>:</td><td>\\s*(\\d+) out of (\\d+).*Specificity</a></b>:</td><td> (\\d+)%.*Sensitivity</a></b>:</td><td> (\\d+)%");
  
  if (rx.indexIn(info) > -1) {
    for (int i = 1; i <= 9; i++)
      niceInfo = niceInfo.arg(rx.cap(i));
    emit movieId(rx.cap(2));
    emit userInfo(niceInfo);
  }
}

void StardustConnector::responseHeaderReceived(const QHttpResponseHeader &resp)
{
  if (resp.statusCode() != 200) {
    emitError();
    return;
  }
  
  switch (currentState) {
    case Connecting:
    {
      QRegExp rx("(PHPSESSID=[a-f0-9]+);");
      if (resp.hasKey("Set-Cookie"))
        if (rx.indexIn(resp.value("Set-Cookie")) > -1)
          cookie = rx.cap(1);
    }
    default:;
  }
}

int StardustConnector::post(const QString & path, 
                            const QMap<QString, QString> &params,
                            QIODevice * to)
{
  QByteArray postParams;
  for (QMap<QString, QString>::const_iterator it = params.constBegin();
       it != params.constEnd(); ++it)
    postParams.append(QUrl::toPercentEncoding(it.key()) + "="
        + QUrl::toPercentEncoding(it.value()) + "&");
  
  QHttpRequestHeader header("POST", path);
  header.setValue("Content-Type", "application/x-www-form-urlencoded");
  header.setValue("Host", host);
  return request(header, postParams, to);
}

int StardustConnector::get(const QString &path, const QString &cookie, QIODevice *to)
{
  QHttpRequestHeader header("GET", path);
  header.setValue("Cookie", cookie);
  header.setValue("Host", host);
  return request(header, QByteArray(), to);
}

int StardustConnector::auth_get(const QString &path, QIODevice *to)
{
  return get(path, cookie, to);
}

StardustConnector::~StardustConnector()
{
}


/*!
    \fn StardustConnector::logout()
    Log out from the server. If not connected, resets state.
    (Really just loses the cookie.)
 */
void StardustConnector::logout()
{
  cookie = QString();
  currentId = 0;
  respPath = QString();
  if (currentState == Connected)
    emit disconnected();
  currentState = NotConnected;
}
