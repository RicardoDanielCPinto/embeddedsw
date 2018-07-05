/*
 * Copyright (C) 2014 - 2019 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 */

/*********************************************************************
 * Pm Node related structures and definitions
 *********************************************************************/

#ifndef PM_NODE_H_
#define PM_NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pm_defs.h"
#include "pm_common.h"
#include "xil_types.h"
#include "xstatus.h"

typedef u8 PmNodeId;
typedef u8 PmStateId;

/* Forward declaration */
typedef struct PmPower PmPower;
typedef struct PmClockHandle PmClockHandle;
typedef struct PmNode PmNode;
typedef struct PmSlave PmSlave;
typedef struct PmProc PmProc;
typedef struct PmNodeClass PmNodeClass;

/* Function pointer for wake/sleep transition functions */
typedef int (*const PmNodeTranHandler)(PmNode* const nodePtr);

/*********************************************************************
 * Macros
 ********************************************************************/

#define NODE_CLASS_PROC		1U
#define NODE_CLASS_POWER	2U
#define NODE_CLASS_SLAVE	3U
#define NODE_CLASS_PLL		4U

#define NODE_IS_PROC(nodePtr)	(NODE_CLASS_PROC == (nodePtr)->class->id)
#define NODE_IS_POWER(nodePtr)	(NODE_CLASS_POWER == (nodePtr)->class->id)
#define NODE_IS_SLAVE(nodePtr)	(NODE_CLASS_SLAVE == (nodePtr)->class->id)

#define NODE_IS_OFF(nodePtr)     (0U == ((nodePtr)->currState & 1U))

#define NODE_LOCKED_POWER_FLAG	0x1U
#define NODE_LOCKED_CLOCK_FLAG	0x2U
#define NODE_IDLE_DONE			0x4U

#define DEFINE_NODE_BUCKET(b)	.bucket = (b), \
				.bucketSize = ARRAY_SIZE(b)

#define DEFINE_PM_POWER_INFO(i)	.powerInfo = (i), \
				.powerInfoCnt = ARRAY_SIZE(i)

#if defined(PM_LOG_LEVEL) && (PM_LOG_LEVEL > 0)
#define DEFINE_NODE_NAME(n)	.name = n
#else
#define DEFINE_NODE_NAME(n)	.name = ""
#endif
/*********************************************************************
 * Structure definitions
 ********************************************************************/

/**
 * PmNode - Structure common for all entities that have node id
 * @derived     Pointer to a derived node structure
 * @class	Pointer to the node class of the node
 * @parent      Pointer to power parent node
 * @clocks      Pointer to the list of clocks that the node uses
 * @latencyMarg Latency margin: lowest latency requirement - powerup latency
 * @nodeId      Node id defined in pm_defs.h
 * @typeId      Type id, used to distinguish the nodes
 * @currState   Id of the node's current state. Interpretation depends on type
 *              of the node, bit 0 value is reserved for off states
 * @powerInfo   Pointer to the array of power consumptions arranged by
 *              stateId
 * @powerInfoCnt  Number of power consumptions in powerInfo array based on
 *                number of states
 * @flags       Node flags
 * @name	Node name
 */
typedef struct PmNode {
	void* const derived;
	PmNodeClass* const class;
	PmPower* const parent;
	PmClockHandle* clocks;
	const u32 *const powerInfo;
	const u32 powerInfoCnt;
	u32 latencyMarg;
	const char* const name;
	const PmNodeId nodeId;
	PmStateId currState;
	u8 flags;
} PmNode;

/**
 * PmNodeClass - Node class models common behavior for a collection of nodes
 * @clearConfig		Clear current configuration of the node
 * @construct		Constructor for the node, call only once on startup
 * @getWakeUpLatency	Get wake-up latency of the node
 * @getPowerData	Get power consumption of the node
 * @forceDown		Put node in the lowest power state
 * @init		Initialize the node
 * @isUsable		Check if the node is usable by current configuration
 * @getPerms		Get permissions (ORed masks of masters allowed to
 *			control node's clocks)
 * @bucket		Pointer to the array of nodes from the class
 * @bucketSize		Number of nodes in the bucket
 * @id			Nodes' class/type ID
 */
typedef struct PmNodeClass {
	void (*const clearConfig)(PmNode* const node);
	void (*const construct)(PmNode* const node);
	s32 (*const getWakeUpLatency)(const PmNode* const node, u32* const lat);
	s32 (*const getPowerData)(const PmNode* const node, u32* const data);
	s32 (*const forceDown)(PmNode* const node);
	s32 (*const init)(PmNode* const node);
	bool (*const isUsable)(PmNode* const node);
	u32 (*const getPerms)(const PmNode* const node);
	PmNode** const bucket;
	const u32 bucketSize;
	const u8 id;
} PmNodeClass;

/*********************************************************************
 * Function declarations
 ********************************************************************/
PmNode* PmGetNodeById(const u32 nodeId);
void* PmNodeGetDerived(const u8 nodeClass, const u32 nodeId);

static inline void* PmNodeGetSlave(const u32 nodeId) {
	return PmNodeGetDerived(NODE_CLASS_SLAVE, nodeId);
}
static inline void* PmNodeGetPower(const u32 nodeId) {
	return PmNodeGetDerived(NODE_CLASS_POWER, nodeId);
}
static inline void* PmNodeGetProc(const u32 nodeId) {
	return PmNodeGetDerived(NODE_CLASS_PROC, nodeId);
}
static inline void* PmNodeGetPll(const u32 nodeId) {
	return PmNodeGetDerived(NODE_CLASS_PLL, nodeId);
}

void PmNodeUpdateCurrState(PmNode* const node, const PmStateId newState);
void PmNodeClearConfig(void);
void PmNodeConstruct(void);
void PmNodeForceDownUnusable(void);
void PmNodeLogUnknownState(const PmNode* const node, const PmStateId state);

s32 PmNodeGetPowerInfo(const PmNode* const node, u32* const data);
s32 PmNodeForceDown(PmNode* const node);
s32 PmNodeInit(void);

u32 PmNodeGetPermissions(PmNode* const node);

#ifdef __cplusplus
}
#endif

#endif /* PM_NODE_H_ */
