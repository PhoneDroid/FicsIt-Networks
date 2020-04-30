#include "FINCodeableSplitter.h"

AFINCodeableSplitter::AFINCodeableSplitter() {
	RootComponent->SetMobility(EComponentMobility::Static);
	NetworkConnector = CreateDefaultSubobject<UFINNetworkConnector>("NetworkConnector");
	NetworkConnector->AddMerged(this);
	NetworkConnector->SetupAttachment(RootComponent);
	NetworkConnector->SetMobility(EComponentMobility::Static);

	InputConnector = CreateDefaultSubobject<UFGFactoryConnectionComponent>("InputConnector");
	InputConnector->SetDirection(EFactoryConnectionDirection::FCD_INPUT);
	InputConnector->SetupAttachment(RootComponent);
	InputConnector->SetMobility(EComponentMobility::Static);
	Output1 = CreateDefaultSubobject<UFGFactoryConnectionComponent>("Output1");
	Output1->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);
	Output1->SetupAttachment(RootComponent);
	Output1->SetMobility(EComponentMobility::Static);
	Output2 = CreateDefaultSubobject<UFGFactoryConnectionComponent>("Output2");
	Output2->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);
	Output2->SetupAttachment(RootComponent);
	Output2->SetMobility(EComponentMobility::Static);
	Output3 = CreateDefaultSubobject<UFGFactoryConnectionComponent>("Output3");
	Output3->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);
	Output3->SetupAttachment(RootComponent);
	Output3->SetMobility(EComponentMobility::Static);
}

void AFINCodeableSplitter::AddListener_Implementation(FFINNetworkTrace listener) {
	SignalListeners.Add(listener);
}

void AFINCodeableSplitter::RemoveListener_Implementation(FFINNetworkTrace listener) {
	SignalListeners.Remove(listener);
}

TSet<FFINNetworkTrace> AFINCodeableSplitter::GetListeners_Implementation() {
	return SignalListeners;
}

void AFINCodeableSplitter::Factory_Tick(float dt) {
	if (InputQueue.Num() < 2) {
		FInventoryItem item;
		float offset;
		if (InputConnector->Factory_GrabOutput(item, offset)) {
			InputQueue.Add(item);
			netSig_ItemRequest(item.ItemClass);
		}
	}
}

bool AFINCodeableSplitter::Factory_PeekOutput_Implementation(const UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const {
	const TArray<FInventoryItem>& outputQueue = GetOutput(connection);
	if (outputQueue.Num() > 0) {
		out_items.Add(outputQueue[0]);
		return true;
	}
	return false;
}

bool AFINCodeableSplitter::Factory_GrabOutput_Implementation(UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) {
	TArray<FInventoryItem>& outputQueue = GetOutput(connection);
	if (outputQueue.Num() > 0) {
		out_item = outputQueue[0];
		outputQueue.RemoveAt(0);
		return true;
	}
	return false;
}

bool AFINCodeableSplitter::netFunc_transferItem(int output) {
	TArray<FInventoryItem>& outputQueue = GetOutput(output);

	if (outputQueue.Num() < 2 &&  InputQueue.Num() > 0) {
		FInventoryItem item = InputQueue[0];
		InputQueue.RemoveAt(0);
		outputQueue.Add(item);
		return true;
	}
	return false;
}

UClass* AFINCodeableSplitter::netFunc_getInput() {
	if (InputQueue.Num() > 0) {
		return InputQueue[0].ItemClass;
	}
	return nullptr;
}

bool AFINCodeableSplitter::netFunc_canOutput(int output) {
	TArray<FInventoryItem>& outputQueue = GetOutput(output);
	return outputQueue.Num() < 2;
}

void AFINCodeableSplitter::netSig_ItemRequest_Implementation(UClass* item) {}

TArray<FInventoryItem>& AFINCodeableSplitter::GetOutput(int output) {
	output = (output < 0) ? 0 : ((output > 2) ? 2 : output);
	switch (output) {
	case 0:
		return OutputQueue1;
		break;
	case 1:
		return OutputQueue2;
		break;
	default:
		return OutputQueue3;
		break;
	}
}

TArray<FInventoryItem>& AFINCodeableSplitter::GetOutput(UFGFactoryConnectionComponent* connection) {
	return const_cast<TArray<FInventoryItem>&>(GetOutput((const UFGFactoryConnectionComponent*) connection));
}

const TArray<FInventoryItem>& AFINCodeableSplitter::GetOutput(const UFGFactoryConnectionComponent* connection) const {
	if (connection == Output1) {
		return OutputQueue1;
	} else if (connection == Output2) {
		return OutputQueue2;
	} else {
		return OutputQueue3;
	}
}