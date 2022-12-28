/*
    Quickddit - Reddit client for mobile phones
    Copyright (C) 2015  Sander van Grieken

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

#include <QDebug>

#include "inboxmanager.h"
#include "settings.h"
#include "messageobject.h"
#include "messagemodel.h"

#define TIMER_INI_INTERVAL 15 * 1000
#define TIMER_MIN_INTERVAL 60 * 1000
#define TIMER_MAX_INTERVAL 30 * 60 * 1000

/*
 * use cases
 * 1. has any unread message.
 * 2. notifications. emit event on *new* unread message.
 */
InboxManager::InboxManager(QObject *parent) :
    AbstractManager(parent), m_enabled(true)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(TIMER_INI_INTERVAL);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, SIGNAL(timeout()), this, SLOT(pollTimeout()));
    m_pollTimer->start();
}

bool InboxManager::hasUnread()
{
    return m_hasUnread;
}

void InboxManager::setHasUnread(bool hasUnread)
{
    if (hasUnread != m_hasUnread) {
        m_hasUnread = hasUnread;
        emit hasUnreadChanged(hasUnread);
    }
}

bool InboxManager::enabled()
{
    return m_enabled;
}

void InboxManager::setEnabled(bool enabled)
{
    if (enabled != m_enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
        if (enabled)
            resetTimer();
    }
}

void InboxManager::pollTimeout()
{
    int interval = m_pollTimer->interval();
    // exponential back-off to max interval
    interval = qMin(interval * 3, TIMER_MAX_INTERVAL);
    // interval lower bound
    interval = qMax(interval, TIMER_MIN_INTERVAL);

    m_pollTimer->setInterval(interval);

    if (m_enabled)
        request();
}

void InboxManager::resetTimer(){
    qDebug();
    m_pollTimer->setInterval(TIMER_INI_INTERVAL);
}

void InboxManager::request()
{
    if (!manager()->isSignedIn())
        return;

    QHash<QString, QString> parameters;
    parameters["mark"] = "false";
    parameters["limit"] = "1";

    doRequest(APIRequest::GET, "/message/unread", SLOT(onUnreadReceived(QNetworkReply*)), parameters);
}

void InboxManager::onUnreadReceived(QNetworkReply *reply)
{
    if (reply != 0) {
        if (reply->error() == QNetworkReply::NoError) {
            const Listing<MessageObject> messageList = Parser::parseMessageList(reply->readAll());
            if (!messageList.isEmpty()) {
                setHasUnread(true);
            } else {
                setHasUnread(false);
            }
        } else {
            emit error(reply->errorString());
        }
    }

    if (hasUnread())
        checkNewUnread();
}

void InboxManager::checkNewUnread()
{
    QHash<QString, QString> parameters;
    parameters["mark"] = "false";
    parameters["limit"] = "10";

    doRequest(APIRequest::GET, "/message/inbox", SLOT(onInboxReceived(QNetworkReply*)), parameters);
}

void InboxManager::onInboxReceived(QNetworkReply *reply)
{
    if (reply != 0) {
        if (reply->error() == QNetworkReply::NoError) {
            const Listing<MessageObject> messageList = Parser::parseMessageList(reply->readAll());
            if (!messageList.isEmpty()) {
                filterInbox(messageList);
            }
        } else {
            emit error(reply->errorString());
        }
    }
}

void InboxManager::filterInbox(Listing<MessageObject> messages)
{
    QString lastSeenMessage = manager()->settings()->lastSeenMessage();

    QVariantList unreadMessages;

    int unreadSinceLastSeen = 0;
    QString newLastSeenMessageId = "";
    const MessageObject* newestUnread = 0;

    foreach(const MessageObject& message, messages) {
        // first in list is newest last-seen
        if (newLastSeenMessageId == "")
            newLastSeenMessageId = message.fullname();
        // stop when we find old last-seen
        if (message.fullname() == lastSeenMessage)
            break;
        // count new unread
        if (message.isUnread()) {
            unreadSinceLastSeen++;
            if (newestUnread == 0)
                newestUnread = &message;
            unreadMessages.append(MessageModel::toMessageVariantMap(message));
        }
    }

    if (unreadSinceLastSeen) {
        resetTimer();
        emit newUnread(unreadSinceLastSeen, unreadMessages);
    }

    manager()->settings()->setLastSeenMessage(newLastSeenMessageId);
}
