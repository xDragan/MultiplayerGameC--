#pragma once

#include "ModuleNetworking.h"
#include "ReplicationManagerServer.h"
#include "DeliveryManager.h"

class ModuleNetworkingServer : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingServer public methods
	//////////////////////////////////////////////////////////////////////

	void setListenPort(int port);



private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isServer() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;



	//////////////////////////////////////////////////////////////////////
	// Client proxies
	//////////////////////////////////////////////////////////////////////

	uint32 nextClientId = 0;

	struct ClientProxy
	{
		bool connected = false;
		sockaddr_in address;
		uint32 clientId;
		std::string name;
		GameObject *gameObject = nullptr;
		double lastPacketReceivedTime = 0.0f;
		float secondsSinceLastReplication = 0.0f;

		ReplicationManagerServer replicationServer;

		uint32 nextExpectedInputSequenceNumber = 0;
		InputController gamepad;
	};

	ClientProxy clientProxies[MAX_CLIENTS];

	ClientProxy * createClientProxy();

	ClientProxy * getClientProxy(const sockaddr_in &clientAddress);

	void destroyClientProxy(ClientProxy * proxy);



public:

	//////////////////////////////////////////////////////////////////////
	// Spawning network objects
	//////////////////////////////////////////////////////////////////////

	GameObject * spawnPlayer(ClientProxy &clientProxy, uint8 spaceshipType);

	GameObject * spawnBullet(GameObject *parent);

	// NOTE(jesus): Here go spawn methods for each type of network objects
	void disconnectUser(GameObject* user);


private:

	DeliveryManager delivery_manager;

	//////////////////////////////////////////////////////////////////////
	// Updating / destroying network objects
	//////////////////////////////////////////////////////////////////////

	void destroyNetworkObject(GameObject *gameObject);
	friend void (NetworkDestroy)(GameObject *);

	void updateNetworkObject(GameObject *gameObject);
	friend void (NetworkUpdate)(GameObject *);

	friend void (NetworkDisconnect)(uint32);

	//////////////////////////////////////////////////////////////////////
	// State
	//////////////////////////////////////////////////////////////////////

	enum class ServerState
	{
		Stopped,
		Listening
	};

	ServerState state = ServerState::Stopped;

	uint16 listenPort = 0;

	float secondsSinceLastPing = 0.0f;
	float replicationDeliveryIntervalSeconds = 0.1f;
};


// NOTE(jesus): It marks an object for replication updates
void NetworkUpdate(GameObject *gameObject);

// NOTE(jesus): For network objects, use this version instead of
// the default Destroy(GameObject *gameObject) one. This one makes
// sure to notify the destruction of the object to all connected
// machines.
void NetworkDestroy(GameObject *gameObject);

class ServerDeliveryDelegate : public DeliveryDelegate {
public:

	void onDeliverySuccess(DeliveryManager* deliveryManager) override;
	void onDeliveryFailure(DeliveryManager* deliveryManager) override;

};
