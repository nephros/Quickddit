/*
    Quickddit - Reddit client for mobile phones
    Copyright (C) 2014  Dickson Leong
    Copyright (C) 2015-2018  Sander van Grieken

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
*/

#ifndef QUICKDDITMANAGER_H
#define QUICKDDITMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>

#include "apirequest.h"

// TODO: rewrite this whole class

class QNetworkAccessManager;
class Settings;

class QuickdditManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool isSignedIn READ isSignedIn WRITE setSignedIn NOTIFY signedInChanged)
    Q_PROPERTY(Settings* settings READ settings WRITE setSettings)
public:
    explicit QuickdditManager(QObject *parent = 0);

    bool isBusy() const;
    void setBusy(const bool busy);

    bool isSignedIn() const;
    void setSignedIn(const bool signedIn);

    Settings *settings() const;
    void setSettings(Settings *settings);

    /**
     * Create a GET request to non-Reddit API
     */
    APIRequest *createGetRequest(QObject *parent, const QUrl &url, const QByteArray &authHeader);

    /**
     * Create a GET/POST request to Reddit API
     * @param parent parent of the APIRequest object
     * @param type specify GET or POST request
     * @param relativeUrl the relative url of Reddit without the ".json", eg. "/r/nokia/hot"
     * @param parameters parameters for the request (will be added as query items in the url)
     */
    APIRequest *createRedditRequest(QObject *parent, APIRequest::HttpMethod method, const QString &relativeUrl,
                             const QHash<QString, QString> &parameters = QHash<QString,QString>());

    /**
     * Generate an authorization url for signing in by the user (through the broswer)
     */
    Q_INVOKABLE QUrl generateAuthorizationUrl();

    /**
     * Get access token after user have signed in
     * @param signedInUrl the redirected url after user have signed in
     */
    Q_INVOKABLE void getAccessToken(const QUrl &signedInUrl);

    /**
     * Remove the access token and refresh token
     */
    Q_INVOKABLE void signOut();

    Q_INVOKABLE void selectAccount(QString accountName);

signals:
    void signedInChanged();
    void accessTokenSuccess();
    void accessTokenFailure(int code, const QString &errorString);
    void busyChanged();

private slots:
    void onAccessTokenRequestFinished(QNetworkReply *reply);
    void onRefreshTokenFinished();
    void onUserInfoFinished(QNetworkReply *reply);
    void onUseTorChanged();

private:
    QNetworkAccessManager *m_netManager;
    Settings *m_settings;

    bool m_busy;
    bool m_signedIn;

    QString m_state;
    APIRequest *m_accessTokenRequest;
    QString m_accessToken;
    QTime m_accessTokenExpiry;

    APIRequest *m_pendingRequest;
    APIRequest *m_userInfoReply;

    void refreshAccessToken();
    void updateRedditUsername();
    void saveOrAddAccountInfo();
};

#endif // QUICKDDITMANAGER_H
