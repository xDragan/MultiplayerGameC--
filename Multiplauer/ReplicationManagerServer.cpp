#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	commands.push_back(std::pair<uint32, ReplicationCommand>(networkId, ReplicationCommand(networkId, ReplicationAction::Create)));
}

void ReplicationManagerServer::update(uint32 networkId)
{
	commands.push_back(std::pair<uint32, ReplicationCommand>(networkId, ReplicationCommand(networkId, ReplicationAction::Update)));
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	commands.push_back(std::pair<uint32, ReplicationCommand>(networkId, ReplicationCommand(networkId, ReplicationAction::Destroy)));
}

void ReplicationManagerServer::write(OutputMemoryStream& packet)
{
	for (int i = 0; i < commands.size(); i++)// std::vector<std::pair<uint32, ReplicationCommand>>::iterator iterator = commands.begin(); iterator != commands.end(); ++iterator)
	{
		uint32 networkId = commands[i].second.networkId;
		ReplicationAction action = commands[i].second.action;

		switch (commands[i].second.action)
		{
		case ReplicationAction::None:
			break;

		case ReplicationAction::Create:
		{
			GameObject* new_game_object = App->modLinkingContext->getNetworkGameObject(commands[i].second.networkId);

			if (new_game_object != nullptr)
			{
				packet << true;

				packet << networkId;
				packet << action;

				packet << new_game_object->angle;
				packet << new_game_object->color.r;
				packet << new_game_object->color.g;
				packet << new_game_object->color.b;
				packet << new_game_object->color.a;
				packet << new_game_object->networkId;
				packet << new_game_object->order;
				packet << new_game_object->pivot.x;
				packet << new_game_object->pivot.y;
				packet << new_game_object->position.x;
				packet << new_game_object->position.y;
				packet << new_game_object->size.x;
				packet << new_game_object->size.y;
				packet << new_game_object->state;
				packet << new_game_object->tag;
				packet << new_game_object->texture->id;
			}
		}
			break;
		case ReplicationAction::Update:
		{
			GameObject* new_game_object = App->modLinkingContext->getNetworkGameObject(commands[i].second.networkId);

			if (new_game_object != nullptr)
			{
				packet << true;

				packet << networkId;
				packet << action;

				packet << new_game_object->angle;
				packet << new_game_object->color.r;
				packet << new_game_object->color.g;
				packet << new_game_object->color.b;
				packet << new_game_object->color.a;
				packet << new_game_object->networkId;
				packet << new_game_object->order;
				packet << new_game_object->pivot.x;
				packet << new_game_object->pivot.y;
				packet << new_game_object->position.x;
				packet << new_game_object->position.y;
				packet << new_game_object->size.x;
				packet << new_game_object->size.y;
				packet << new_game_object->state;
				packet << new_game_object->tag;

			}
		}
		break;
		case ReplicationAction::Destroy:
		{
			packet << true;

			packet << networkId;
			packet << action;
		}
			break;
		}

	}

	packet << false;

	commands.clear();
}
