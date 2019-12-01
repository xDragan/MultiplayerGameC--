#include "Networks.h"
#include "DeliveryManager.h"

Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
	Delivery* new_delivery = new Delivery();

	new_delivery->sequenceNumber = last_index;
	new_delivery->dispatchTime = Time.time;

	deliveries.push_back(new_delivery);

	packet << last_index;

	last_index += 1;

	return new_delivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
	bool ret = true;

	uint32 new_index = 0;
	packet >> new_index;

	if (pending_acknowledgements.empty())
	{
		pending_acknowledgements.push_back(new_index);
	}
	else
	{
		if (new_index != pending_acknowledgements.at(pending_acknowledgements.size() - 1) + 1)
		{
			ret = false;
		}
		else
		{
			pending_acknowledgements.push_back(new_index);
		}
	}

	return ret;
}

bool DeliveryManager::hasSequenceNumberPendingAck() const
{
	return !pending_acknowledgements.empty();
}

void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream& packet)
{
	for (int i = 0; i < pending_acknowledgements.size(); i++) {
		packet << pending_acknowledgements[i];
	}
	pending_acknowledgements.clear();
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > 0) {
		uint32 recieved_seq = 0;
		packet >> recieved_seq;

		for (std::vector<Delivery*>::iterator iterator = deliveries.begin(); iterator != deliveries.end(); iterator++)
		{
			if (recieved_seq == (*iterator._Ptr)->sequenceNumber) {
				(*iterator._Ptr)->delegate->onDeliverySuccess(this);
				
				delete (*iterator._Ptr)->delegate;
				delete (*iterator._Ptr);
				
				deliveries.erase(iterator);

				break;
			}
		}
	}
}

void DeliveryManager::processTimedOutPackets()
{
	for (std::vector<Delivery*>::iterator iterator = deliveries.begin(); iterator != deliveries.end(); iterator++)
	{
		if (Time.time - (*iterator._Ptr)->dispatchTime > 5.0f)
		{
			(*iterator._Ptr)->delegate->onDeliveryFailure(this);
			
			delete (*iterator._Ptr)->delegate;
			delete (*iterator._Ptr);

			deliveries.erase(iterator);
			
			break;
		}
	}
}
