#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	bool command_exist = false;

	packet >> command_exist;

	while(command_exist)
	{
		uint32 networkId = 0;
		ReplicationAction action = ReplicationAction::None;

		packet >> networkId;
		packet >> action;

		switch (action)
		{
		case ReplicationAction::Create:
		{
			if (App->modLinkingContext->getNetworkGameObject(networkId) != nullptr)
			{
				break;
			}

			GameObject* new_game_object = App->modGameObject->Instantiate();

			packet >> new_game_object->angle;
			packet >> new_game_object->color.r;
			packet >> new_game_object->color.g;
			packet >> new_game_object->color.b;
			packet >> new_game_object->color.a;
			packet >> new_game_object->networkId;
			packet >> new_game_object->order;
			packet >> new_game_object->pivot.x;
			packet >> new_game_object->pivot.y;
			packet >> new_game_object->position.x;
			packet >> new_game_object->position.y;
			packet >> new_game_object->size.x;
			packet >> new_game_object->size.y;
			packet >> new_game_object->state;
			packet >> new_game_object->tag;
			uint32 texture_id = 0;
			packet >> texture_id;

			new_game_object->texture = (Texture*)App->modTextures->GetTextureByID(texture_id);
			App->modLinkingContext->registerNetworkGameObjectWithNetworkId(new_game_object, networkId);
		}
		break;
		case ReplicationAction::Update:
		{
			GameObject* new_game_object = App->modLinkingContext->getNetworkGameObject(networkId);

			packet >> new_game_object->angle;
			packet >> new_game_object->color.r;
			packet >> new_game_object->color.g;
			packet >> new_game_object->color.b;
			packet >> new_game_object->color.a;
			packet >> new_game_object->networkId;
			packet >> new_game_object->order;
			packet >> new_game_object->pivot.x;
			packet >> new_game_object->pivot.y;
			packet >> new_game_object->position.x;
			packet >> new_game_object->position.y;
			packet >> new_game_object->size.x;
			packet >> new_game_object->size.y;
			packet >> new_game_object->state;
			packet >> new_game_object->tag;
		}
		break;
		case ReplicationAction::Destroy:
		{
			GameObject* new_game_object = App->modLinkingContext->getNetworkGameObject(networkId);
			if (new_game_object != nullptr)
			{
				App->modLinkingContext->unregisterNetworkGameObject(new_game_object);
				App->modGameObject->Destroy(new_game_object);
			}
		}
		break;
		}

		packet >> command_exist;
	}

}
