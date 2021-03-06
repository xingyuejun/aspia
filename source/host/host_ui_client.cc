//
// Aspia Project
// Copyright (C) 2019 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "host/host_ui_client.h"
#include "common/message_serialization.h"
#include "host/host_ui_constants.h"
#include "ipc/ipc_channel.h"

namespace host {

UiClient::UiClient(QObject* parent)
    : QObject(parent)
{
    // Nothing
}

UiClient::~UiClient() = default;

void UiClient::start()
{
    state_ = State::CONNECTING;
    channel_ = ipc::Channel::createClient(this);

    connect(channel_, &ipc::Channel::connected, [this]()
    {
        state_ = State::CONNECTED;
        emit connected();

        channel_->start();
    });

    connect(channel_, &ipc::Channel::disconnected, this, &UiClient::stop);
    connect(channel_, &ipc::Channel::errorOccurred, this, &UiClient::errorOccurred);
    connect(channel_, &ipc::Channel::messageReceived, this, &UiClient::onChannelMessage);

    channel_->connectToServer(kIpcChannelIdForUI);
}

void UiClient::stop()
{
    if (state_ == State::NOT_CONNECTED)
        return;

    if (channel_)
        channel_->stop();

    state_ = State::NOT_CONNECTED;
    emit disconnected();
}

void UiClient::refresh()
{
    if (!channel_)
        return;

    proto::host::UiToService message;

    message.mutable_credentials_request()->set_flags(
        proto::host::CredentialsRequest::REFRESH);

    channel_->send(common::serializeMessage(message));
}

void UiClient::newPassword()
{
    if (!channel_)
        return;

    proto::host::UiToService message;

    message.mutable_credentials_request()->set_flags(
        proto::host::CredentialsRequest::NEW_PASSWORD);

    channel_->send(common::serializeMessage(message));
}

void UiClient::killSession(const std::string& uuid)
{
    if (!channel_)
        return;

    proto::host::UiToService message;
    message.mutable_kill_session()->set_uuid(uuid);
    channel_->send(common::serializeMessage(message));
}

void UiClient::onChannelMessage(const QByteArray& buffer)
{
    proto::host::ServiceToUi message;
    if (!common::parseMessage(buffer, message))
    {
        LOG(LS_WARNING) << "Invalid message from service";
        stop();
        return;
    }

    if (message.has_credentials())
    {
        emit credentialsReceived(message.credentials());
    }
    else if (message.has_connect_event())
    {
        emit connectEvent(message.connect_event());
    }
    else if (message.has_disconnect_event())
    {
        emit disconnectEvent(message.disconnect_event());
    }
    else
    {
        LOG(LS_WARNING) << "Unhandled message from service";
    }
}

} // namespace host
