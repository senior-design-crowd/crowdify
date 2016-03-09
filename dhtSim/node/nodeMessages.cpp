#include "nodeMessages.h"

namespace NodeToRootMessage {
	MPI_Datatype MPI_NodeToNodeMsg;
	MPI_Datatype MPI_NodeNeighborUpdate;
}

namespace InterNodeMessage {
	MPI_Datatype MPI_DHTRegion;
	MPI_Datatype MPI_DHTPointOwnerRequest;
	MPI_Datatype MPI_DHTPointOwnerResponse;
	MPI_Datatype MPI_DHTAreaResponse;
	MPI_Datatype MPI_NodeTakeoverInfo;
	MPI_Datatype MPI_NodeTakeoverResponse;
}

bool InitMPITypes()
{
	int ret = -1;

	// MPI_DHTRegion
	{
		/*
		typedef struct _DHTRegion {
		float left, right, top, bottom;
		} DHTRegion;
		*/

		MPI_Datatype types[] = { MPI_FLOAT };
		int blockSizes[] = { 4 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTRegion);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTRegion);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeToNodeMsg
	{
		/*typedef struct {
		int										otherNode;
		NodeToNodeMsgTypes::NodeToNodeMsgType	msgType;
		} NodeToNodeMsg;*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &NodeToRootMessage::MPI_NodeToNodeMsg);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&NodeToRootMessage::MPI_NodeToNodeMsg);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeNeighborUpdate
	{
		/*typedef struct {
		int neighbors[10];
		int numNeighbors;
		} NodeNeighborUpdate;*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 11 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &NodeToRootMessage::MPI_NodeNeighborUpdate);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&NodeToRootMessage::MPI_NodeNeighborUpdate);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTPointOwnerRequest
	{
		/*
		typedef struct
		{
		float	x, y;
		int		origRequestingNode;
		} DHTPointOwnerRequest;
		*/

		MPI_Datatype types[] = { MPI_FLOAT, MPI_INT };
		int blockSizes[] = { 2, 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(sizeof(float) * 2)
		};

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTPointOwnerRequest);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTPointOwnerRequest);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTPointOwnerResponse
	{
		/*
		typedef struct
		{
		int pointOwnerNode;
		} DHTPointOwnerResponse;
		*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTPointOwnerResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTPointOwnerResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTAreaResponse
	{
		/*
		typedef struct {
		DHTRegion		newDHTArea;
		int				numNeighbors;
		int				nextAxisToSplit;
		} DHTAreaResponse;
		*/

		MPI_Datatype types[] = { InterNodeMessage::MPI_DHTRegion, MPI_INT };
		int blockSizes[] = { 1, 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(0)
		};

		MPI_Aint lb;
		ret = MPI_Type_get_extent(InterNodeMessage::MPI_DHTRegion, &lb, &displacements[1]);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTAreaResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTAreaResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeTakeoverInfo
	{
		/*
		typedef struct
		{
		int				nodeNum;
		DHTRegion		myDHTArea;
		} NodeTakeoverInfo;
		*/

		MPI_Datatype types[] = { MPI_INT, InterNodeMessage::MPI_DHTRegion };
		int blockSizes[] = { 1, 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(sizeof(int))
		};

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeTakeoverInfo);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeTakeoverInfo);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeTakeoverResponse
	{
		/*
		typedef struct
		{
		DHTRegion		myDHTArea;
		int				nodeNum;

		enum ResponseType : unsigned int {
		CAN_TAKEOVER = 1,
		SMALLER_AREA,
		NOT_MY_NEIGHBOR,
		NODE_STILL_ALIVE
		};

		ResponseType	response;
		} NodeTakeoverResponse;
		*/

		MPI_Datatype types[] = { InterNodeMessage::MPI_DHTRegion, MPI_LONG };
		int blockSizes[] = { 1, 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(0)
		};

		MPI_Aint lb;
		ret = MPI_Type_get_extent(InterNodeMessage::MPI_DHTRegion, &lb, &displacements[1]);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeTakeoverResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeTakeoverResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	return true;
}