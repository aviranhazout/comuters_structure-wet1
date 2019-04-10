/* 046267 Computer Architecture - Spring 2019 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

#define max_machines 255
#define max_lines 32


typedef struct history_ {
	int current;
	int max;
} history;

typedef struct state_machine_ {
	unsigned machines[max_machines];
} state_machine;

typedef struct BTB_line_{
	int tag;
	int dest;
	int tag_length;
	history* hist;
	state_machine* states;
} BTB_line;

typedef struct BTB_ {
	BTB_line lines[max_lines];
	int max;
	int share;
	bool isGlobalHist;
	bool isGlobalTable;
} BTB;

BTB table;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	history* hist;
	state_machine* machine_table;

	//allocating history
	if (isGlobalHist)
		hist = (history)malloc(sizeof(history));
	else
		hist = (history)malloc(btbSize*sizeof(history));

	if (hist == NULL)
		return -1;

	//allocating machine tables
	if (isGlobalTable)
		machine_table = (state_machine)malloc(sizeof(state_machine));
	else
		machine_table = (state_machine)malloc(btbSize*sizeof(state_machine));

	if (machine_table == NULL)
		return -1;

	//initializing everything
	table.max = btbSize;
	table.isGlobalHist = isGlobalHist;
	table.isGlobalTable = isGlobalTable;
	for (int i = 0; i < btbSize; i++)
	{
		table.lines[i].tag_length = tagSize;

		if (isGlobalHist)
			table.lines[i].hist = hist;
		else
			table.lines[i].hist = hist + i;

		table.lines[i].hist->max = 2^historySize - 1;
		table.lines[i].hist->current = 0;

		if (isGlobalTable)
			table.lines[i].states = machine_table;
		else
			table.lines[i].states = machine_table + i;

		for(int j = 0; j < 2^historySize; j++)
			table.lines[i].states->machines[j] = fsmState;

		table.share = Shared;
	}

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

