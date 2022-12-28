/*
    Quickddit - Reddit client for mobile phones
    Copyright (C) 2014  Dickson Leong
    Copyright (C) 2015-2017  Sander van Grieken

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

import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.quickddit.Core 1.0

AbstractPage {
    id: messagePage
    objectName: "messagePage"
    title: qsTr("Messages") + " - " + sectionModel[messageModel.section]
    busy: messageManager.busy

    property alias section: messageModel.section

    property bool animateRemove: true

    function refresh() {
        animateRemove = false
        messageModel.refresh(false);
        inboxManager.dismiss();
    }

    readonly property variant sectionModel: [qsTr("All"), qsTr("Unread"), qsTr("Messages"), qsTr("Comment Replies"), qsTr("Post Replies"), qsTr("Sent"), qsTr("Username Mentions")]

    SilicaListView {
        id: messageListView
        anchors.fill: parent
        model: messageModel

        PullDownMenu {
            MenuItem {
                text: qsTr("New Message")
                onClicked: pageStack.push(Qt.resolvedUrl("SendMessagePage.qml"), {messageManager: messageManager});
            }
            MenuItem {
                text: qsTr("Section")
                onClicked: {
                    globalUtils.createSelectionDialog(qsTr("Section"), sectionModel, messageModel.section,
                    function(selectedIndex) {
                        messageModel.section = selectedIndex;
                        settings.messageSection = selectedIndex;
                        messagePage.refresh();
                    });
                }
            }
            MenuItem {
                text: qsTr("Refresh")
                enabled: !messageModel.busy
                onClicked: messagePage.refresh();
            }
        }

        header: QuickdditPageHeader { title: messagePage.title }

        delegate: MessageDelegate {
            id: messageDelegate
            width: parent.width
            animateRemove: messagePage.animateRemove

            listItem.menu: Component { MessageMenu {} }
            listItem.showMenuOnPressAndHold: false
            listItem.onPressAndHold: {
                var dialog = showMenu({message: model, messageManager: messageManager})
                dialog.replyClicked.connect(function() {
                    if (model.author !== settings.redditUsername) {
                        messageDelegate.doReply()
                    }
                });
                dialog.deleteClicked.connect(function() {
                    messageDelegate.remorseAction(qsTr("Deleting message"), function() {
                        animateRemove = true
                        messageManager.del(model.fullname);
                    })
                });
            }

            onClicked: {
                if (model.isComment) {
                    doOpenComment()
                } else if (model.author !== settings.redditUsername) {
                    doReply()
                }
            }

            function doOpenComment() {
                if (model.isUnread) {
                    messageManager.markRead(model.fullname)
                }
                pageStack.push(Qt.resolvedUrl("CommentPage.qml"), {linkPermalink: model.context})
            }

            function doReply() {
                if (model.isUnread) {
                    messageManager.markRead(model.fullname)
                }
                pageStack.push(Qt.resolvedUrl("SendMessagePage.qml"),
                               {messageManager: messageManager, replyTo: model.fullname, recipient: model.author !== "" ? model.author : "/r/" + model.subreddit, subject: "re: " + model.subject});
            }

            AltMarker { }
        }

        footer: LoadingFooter {
            visible: messageModel.busy || (messageListView.count > 0 && messageModel.canLoadMore)
            running: messageModel.busy
            listViewItem: messageListView
        }

        onAtYEndChanged: {
            if (atYEnd && count > 0 && !messageModel.busy && messageModel.canLoadMore)
                messageModel.refresh(true);
        }

        ViewPlaceholder { enabled: messageListView.count == 0 && !messageModel.busy; text: qsTr("Nothing here :(") }

        VerticalScrollDecorator {}
    }

    MessageModel {
        id: messageModel
        manager: quickdditManager
        onError: infoBanner.warning(errorString)
    }

    MessageManager {
        id: messageManager
        manager: quickdditManager
        onReplySuccess: {
            infoBanner.alert(qsTr("Message sent"));
            pageStack.pop();
        }
        onSendSuccess: {
            infoBanner.alert(qsTr("Message sent"));
            pageStack.pop();
        }
        onMarkReadStatusSuccess: messageModel.changeIsUnread(fullname, isUnread);
        onDelSuccess: messageModel.del(fullname);
        onError: infoBanner.warning(errorString);
    }

    Component.onCompleted: {
        inboxManager.dismiss();
    }
}
