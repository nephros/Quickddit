/*
    Quickddit - Reddit client for mobile phones
    Copyright (C) 2014  Dickson Leong

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

#include "commentmanager.h"

#include <QtNetwork/QNetworkReply>

#include "settings.h"
#include "commentmodel.h"
#include "parser.h"

CommentManager::CommentManager(QObject *parent) :
    AbstractManager(parent), m_model(0)
{
}

CommentModel *CommentManager::model() const
{
    return m_model;
}

void CommentManager::setModel(CommentModel *model)
{
    m_model = model;
}

QString CommentManager::linkAuthor() const
{
    return m_linkAuthor;
}

void CommentManager::setLinkAuthor(const QString &linkAuthor)
{
    m_linkAuthor = linkAuthor;
}

void CommentManager::addComment(const QString &replyTofullname, const QString &rawText)
{
    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    parameters.insert("text", rawText);
    parameters.insert("thing_id", replyTofullname);
    m_action = Insert;
    m_fullname = replyTofullname;

    doRequest(APIRequest::POST, "/api/comment", SLOT(onFinished(QNetworkReply*)), parameters);
}

void CommentManager::editComment(const QString &fullname, const QString &rawText)
{
    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    parameters.insert("text", rawText);
    parameters.insert("thing_id", fullname);
    m_action = Edit;
    m_fullname = fullname;

    doRequest(APIRequest::POST, "/api/editusertext", SLOT(onFinished(QNetworkReply*)), parameters);
}

void CommentManager::deleteComment(const QString &fullname)
{
    QHash<QString, QString> parameters;
    parameters.insert("id", fullname);
    m_action = Delete;
    m_fullname = fullname;

    doRequest(APIRequest::POST, "/api/del", SLOT(onFinished(QNetworkReply*)), parameters);
}

void CommentManager::onFinished(QNetworkReply *reply)
{
    if (reply != 0) {
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray json = reply->readAll();

            QList<QString> errors = Parser::parseErrors(json);
            if (errors.size() > 0) {
                emit error(errors.takeFirst(), m_fullname);
            } else {
                if (m_action == Insert || m_action == Edit) {
                    CommentObject comment = Parser::parseNewComment(json);
                    if (m_linkAuthor == manager()->settings()->redditUsername())
                        comment.setSubmitter(true);

                    if (m_action == Insert) {
                        m_model->insertComment(comment, m_fullname);
                        emit success(tr("The comment has been added"), m_fullname);
                    } else {
                        m_model->editComment(comment);
                        emit success(tr("The comment has been edited"), m_fullname);
                    }
                } else if (m_action == Delete) {
                    m_model->deleteComment(m_fullname);
                    emit success(tr("The comment has been deleted"), m_fullname);
                }
            }
        } else {
            emit error(reply->errorString(), m_fullname);
        }
    }

    m_fullname.clear();
}
