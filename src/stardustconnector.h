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
#ifndef STARDUSTCONNECTOR_H
#define STARDUSTCONNECTOR_H

#include <QObject>
#include <QHttp>
#include <QUrl>

/**
	@author Rafał Rzepecki <divided.mind@gmail.com>
*/
class StardustConnector : public QHttp
{
Q_OBJECT
public:
    StardustConnector(QObject *parent = 0);
    void login(const QString &username, const QString &password);
    int post(const QString & path, 
             const QMap<QString, QString> &params,
             QIODevice * to = 0);
    int get(const QString &path, const QString &cookie, QIODevice *to = 0);
    int auth_get(const QString &path, QIODevice *to = 0);

    ~StardustConnector();

public slots:
  void respondNoFocus();
  void respondNoTrack();
  void respondTrack(int x, int y, int z);

protected slots:
  void requestImages(const QString &path = "/ss_virtual_microscope.php");
  
signals:
  void connecting();
  void error();
  void connected();
  void newImage(const QUrl &path, int frames);
  void nextImage(const QUrl &path, int frames);
  void movieId(const QString &id);
  void userInfo(const QString &info);

private slots:
  void requestFinished(int id, bool error);
  void responseHeaderReceived(const QHttpResponseHeader &resp);

private:
  void emitError();
  void parseMicroscope(const QByteArray &page);
  void parseInfo(const QString &info);

  enum State {
    NotConnected,
    Connecting,
    Connected
  };
  
  const QString host;
  State currentState;
  int currentId;
  QString cookie;
  QString respPath;
};

#endif
