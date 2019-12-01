#pragma once

enum class ReplicationAction {
	None, Create, Update, Destroy
};

struct ReplicationCommand 
{
	ReplicationCommand(uint32 new_networkId, ReplicationAction new_action) : networkId(new_networkId), action(new_action) {}

	ReplicationAction action;
	uint32 networkId;
};

class ReplicationManagerServer {
public:
	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);
	
	void write(OutputMemoryStream& packet);

	std::vector<std::pair<uint32, ReplicationCommand>> commands;
};

