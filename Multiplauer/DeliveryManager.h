#pragma once
class DeliveryManager;

class DeliveryDelegate {
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;

};

struct Delivery 
{
	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0;
	DeliveryDelegate* delegate = nullptr;
};

class DeliveryManager {

public:
	
	Delivery* writeSequenceNumber(OutputMemoryStream&);

	bool processSequenceNumber(const InputMemoryStream&);
	
	//For recievers
	bool hasSequenceNumberPendingAck() const;
	void writeSequenceNumbersPendingAck(OutputMemoryStream&);

	//For Senders
	void processAckdSequenceNumbers(const InputMemoryStream&);
	void processTimedOutPackets();

	void clear();

private:

	std::vector<Delivery*> deliveries;
	uint32 last_index = 0;
	std::vector<uint32> pending_acknowledgements;

public:

	uint32 GetLastIndex()const { return last_index; }

};