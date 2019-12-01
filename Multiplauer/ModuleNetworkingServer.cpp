#include "Networks.h"
#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;

	secondsSinceLastPing = 0.0f;
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

		ImGui::Separator();

		ImGui::Text("Replication");
		ImGui::InputFloat("Delivery interval (s)", &replicationDeliveryIntervalSeconds, 0.01f, 0.1f);
		
		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].name != "")
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					ImGui::Text(" - Last packet time: %.04f", clientProxies[i].lastPacketReceivedTime);
					ImGui::Text(" - Seconds since repl.: %.04f", clientProxies[i].secondsSinceLastReplication);
					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		// Register player
		ClientProxy *proxy = getClientProxy(fromAddress);

		// Read the packet type
		ClientMessage message;
		packet >> message;

		// Process the packet depending on its type
		if (message == ClientMessage::Hello)
		{
			bool newClient = false;

			if (proxy == nullptr)
			{
				proxy = createClientProxy();

				newClient = true;

				std::string playerName;
				uint8 spaceshipType;
				packet >> playerName;
				packet >> spaceshipType;

				proxy->address.sin_family = fromAddress.sin_family;
				proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
				proxy->address.sin_port = fromAddress.sin_port;
				proxy->connected = true;
				proxy->name = playerName;
				proxy->clientId = nextClientId++;

				// Create new network object
				spawnPlayer(*proxy, spaceshipType);

				// Send welcome to the new player
				OutputMemoryStream welcomePacket;
				welcomePacket << ServerMessage::Welcome;
				welcomePacket << proxy->clientId;
				welcomePacket << proxy->gameObject->networkId;
				sendPacket(welcomePacket, fromAddress);

				// Send all network objects to the new player
				uint16 networkGameObjectsCount;
				GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
				App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
				
				for (uint16 i = 0; i < networkGameObjectsCount; ++i)
				{
					GameObject *gameObject = networkGameObjects[i];
					
					//if (gameObject->networkId == proxy->clientId)continue;

					// TODO(jesus): Notify the new client proxy's replication manager about the creation of this game object
					proxy->replicationServer.create(gameObject->networkId);
				}

				LOG("Message received: hello - from player %s", playerName.c_str());
			}

			if (!newClient)
			{
				// Send welcome to the new player
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << ServerMessage::Unwelcome;
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello - from player %s", proxy->name.c_str());
			}
		}
		else if (message == ClientMessage::Input)
		{
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr)
			{

				int last_processed_input_data = -1;
				// Read input data
				while (packet.RemainingByteCount() > 0)
				{
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber)
					{
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);
						proxy->gameObject->behaviour->onInput(proxy->gamepad);
						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
						last_processed_input_data = inputData.sequenceNumber;
					}
				}

				if (last_processed_input_data > -1)
				{
					uint32 last_input = last_processed_input_data;
					//Send packet with the last processed input
					OutputMemoryStream processedInputPacket;
					processedInputPacket << ServerMessage::Input;
					processedInputPacket << last_input;
					sendPacket(processedInputPacket, fromAddress);
				}
			}
			
		}
		else if (message == ClientMessage::Delivery) {
			delivery_manager.processAckdSequenceNumbers(packet);
		}
		
		if (proxy != nullptr)
		{
			proxy->lastPacketReceivedTime = Time.time;
		}
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{
		bool last_ping = false;
		if (Time.time - secondsSinceLastPing > PING_INTERVAL_SECONDS) 
		{
			last_ping = true;
			secondsSinceLastPing = Time.time;
		}

		bool replication = false;
		if (Time.time - replicationDeliveryIntervalSeconds > REPLICATION_INTERVAL_SECONDS)
		{
			replication = true;
			replicationDeliveryIntervalSeconds = Time.time;
		}

		// Replication
		for (ClientProxy &clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				if (Time.time - clientProxy.lastPacketReceivedTime > DISCONNECT_TIMEOUT_SECONDS) {
					clientProxy.connected = false;
					destroyNetworkObject(clientProxy.gameObject);
				}

				if (last_ping) 
				{
					OutputMemoryStream ping_packet;
					ping_packet << ServerMessage::Ping;
					sendPacket(ping_packet, clientProxy.address);
				}

				if (replication) 
				{
					if (!clientProxy.replicationServer.commands.empty()) 
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Replication;

						//Create the delivery sequence
						Delivery* new_delivery = delivery_manager.writeSequenceNumber(packet);
						new_delivery->delegate = new ServerDeliveryDelegate();

						clientProxy.replicationServer.write(packet);

						sendPacket(packet, clientProxy.address);
					}


				}
				// TODO(jesus): If the replication interval passed and the replication manager of this proxy
				//              has pending data, write and send a replication packet to this client.
			}
		}

		delivery_manager.processTimedOutPackets();
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);

	if (proxy)
	{
		// Notify game object deletion to replication managers
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientProxies[i].connected && proxy->clientId != clientProxies[i].clientId)
			{
				// TODO(jesus): Notify this proxy's replication manager about the destruction of this player's game object
				proxy->replicationServer.destroy(clientProxies[i].clientId);
			}
		}

		// Unregister the network identity
		App->modLinkingContext->unregisterNetworkGameObject(proxy->gameObject);

		// Remove its associated game object
		Destroy(proxy->gameObject);

		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	// Destroy network game objects
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];
	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}

	// Clear all client proxies
	for (ClientProxy &clientProxy : clientProxies)
	{
		destroyClientProxy(&clientProxy);
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy * proxy)
{
		*proxy = {};
}



//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::spawnPlayer(ClientProxy &clientProxy, uint8 spaceshipType)
{
	// Create a new game object with the player properties
	clientProxy.gameObject = Instantiate();
	clientProxy.gameObject->size = { 100, 100 };
	clientProxy.gameObject->angle = 45.0f;

	if (spaceshipType == 0) {
		clientProxy.gameObject->texture = App->modResources->spacecraft1;
	}
	else if (spaceshipType == 1) {
		clientProxy.gameObject->texture = App->modResources->spacecraft2;
	}
	else {
		clientProxy.gameObject->texture = App->modResources->spacecraft3;
	}
	

	// Create collider
	clientProxy.gameObject->collider = App->modCollision->addCollider(ColliderType::Player, clientProxy.gameObject);
	clientProxy.gameObject->collider->isTrigger = true;

	// Create behaviour
	clientProxy.gameObject->behaviour = new Spaceship;
	clientProxy.gameObject->behaviour->gameObject = clientProxy.gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(clientProxy.gameObject, clientProxy.clientId);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationServer.create(clientProxy.clientId);
		}
	}

	return clientProxy.gameObject;
}

GameObject * ModuleNetworkingServer::spawnBullet(GameObject *parent)
{
	// Create a new game object with the player properties
	GameObject *gameObject = Instantiate();
	gameObject->size = { 20, 60 };
	gameObject->angle = parent->angle;
	gameObject->position = parent->position;
	gameObject->texture = App->modResources->laser;
	gameObject->collider = App->modCollision->addCollider(ColliderType::Laser, gameObject);

	// Create behaviour
	gameObject->behaviour = new Laser;
	gameObject->behaviour->gameObject = gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationServer.create(gameObject->networkId);
		}
	}

	return gameObject;
}


//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the destruction of this game object
			clientProxies[i].replicationServer.destroy(gameObject->networkId);
		}
	}
	
	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely //
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the update of this game object
			clientProxies[i].replicationServer.update(gameObject->networkId);
		}
	}
}

void ModuleNetworkingServer::disconnectUser(GameObject* user)
{
	
}
/*
void ModuleNetworkingServer::alertUserDisconnect(uint32 network_id)
{
	for (ClientProxy& clientProxy : clientProxies)
	{
		if (clientProxy.connected)
		{
			if (clientProxy.gameObject->networkId == network_id)
			{
				OutputMemoryStream packet;
				packet << ServerMessage::Disconnect;
				packet << network_id;

				sendPacket(packet, clientProxy.address);
			}
		}
	}
}*/

//////////////////////////////////////////////////////////////////////
// Global update / destruction of game objects
//////////////////////////////////////////////////////////////////////

void NetworkUpdate(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->updateNetworkObject(gameObject);
}

void NetworkDestroy(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->destroyNetworkObject(gameObject);
}
/*
void NetworkDisconnect(GameObject* gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetClient->disconnect();

}*/

void ServerDeliveryDelegate::onDeliverySuccess(DeliveryManager* deliveryManager)
{
	LOG("PACKET SUCCESS");
}

void ServerDeliveryDelegate::onDeliveryFailure(DeliveryManager* deliveryManager)
{
	LOG("PACKET FAILURE");
}
