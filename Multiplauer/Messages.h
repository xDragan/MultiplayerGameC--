#pragma once

enum class ClientMessage
{
	Hello,
	Input,
	Ping,
	Delivery,
	Request_Disconnect
};

enum class ServerMessage
{
	Welcome,
	Unwelcome,
	Replication,
	Ping,
	Input,
	Disconnect
};
