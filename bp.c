/* 046267 Computer Architecture - Spring 2019 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define max_machines 255
#define max_lines 32

//------------------------
//      structs
//------------------------
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
	history* hist;
	state_machine* states;
} BTB_line;

typedef struct BTB_ {
	BTB_line lines[max_lines];
	int max;
	int share;
	bool isGlobalHist;
	bool isGlobalTable;
	int base_state;
    int tag_length;
} BTB;

//------------------------
//      globals
//------------------------
BTB table;
SIM_stats stats;
bool is_taken;

//------------------------
//      functions
//------------------------
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	history* hist;
	state_machine* machine_table;

    int tables_num, tables_mem, hist_mem;
    tables_num = pow(2,historySize);

    //allocating history
	if (isGlobalHist)
    {
	    hist = (history*)malloc(sizeof(history));
	    hist_mem = historySize;
    }
	else
    {
	    hist = (history*)malloc(btbSize*sizeof(history));
        hist_mem = historySize * btbSize;
    }

	if (hist == NULL)
		return -1;

	//allocating machine tables
	if (isGlobalTable)
    {
	    machine_table = (state_machine*)malloc(sizeof(state_machine));
        tables_mem = 2 * tables_num;
    }
	else
    {
	    machine_table = (state_machine*)malloc(btbSize*sizeof(state_machine));
        tables_mem = 2 * tables_num * btbSize;
    }

	if (machine_table == NULL)
	{
	    free(hist);
        return -1;
    }
	//initializing everything
	table.max = btbSize;
	table.isGlobalHist = isGlobalHist;
	table.isGlobalTable = isGlobalTable;
	table.base_state = fsmState;
    table.tag_length = tagSize;
    for (int i = 0; i < btbSize; i++)
	{
		if (isGlobalHist)
			table.lines[i].hist = hist;
		else
			table.lines[i].hist = hist + i;
        table.lines[i].tag = 0;
        table.lines[i].dest = 0;
		table.lines[i].hist->max = tables_num;
		table.lines[i].hist->current = 0;

		if (isGlobalTable)
			table.lines[i].states = machine_table;
		else
			table.lines[i].states = machine_table + i;

		for(int j = 0; j < tables_num; j++)
			table.lines[i].states->machines[j] = fsmState;

		table.share = Shared;
	}

    // initialize stats
    stats.br_num = 0;
    stats.flush_num = 0;
    stats.size = btbSize * (tagSize + 30) + hist_mem + tables_mem;
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
    int pc_sr2, place, tag, history, machine;
    pc_sr2 = pc/4;
    place = (pc_sr2) % (table.max);
    tag = pc_sr2 % (table.tag_length);
    if (table.lines[place].tag == tag)
    {    //if the tag is found
        history = table.lines[place].hist->current;
        if (table.share == 0)
            machine = history;
        else if (table.share == 1)
            machine = history^(pc_sr2 % (table.lines[place].hist->max));
        else
            machine = history^(int)((pc_sr2 % (table.lines[place].hist->max)) / pow(2,14));
            printf("\n\n%d\n\n",machine);
        if (table.lines[place].states->machines[machine] > 1)
        {   //and if the state is taken
            is_taken = true;
            *dst = 4*(table.lines[place].dest);
            return true;
        }
    }
    //if the tag was not found or the state was not-taken
    is_taken = false;
    *dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
    int pc_sr2, place, history, tag, machine;
    pc_sr2 = pc/4;
    place = pc_sr2 % (table.max);
    tag = pc_sr2 % (table.tag_length);
    bool update_hist, update_table;
    update_hist = true;
    update_table = true;
    if (table.lines[place].tag != tag)
    {   //tag was not found - clean or update the line's history and tables according to global or local
        table.lines[place].tag = tag;
        if (!(table.isGlobalTable))
            update_table = false;

        if (!(table.isGlobalHist))
            update_hist = false;
    }
    table.lines[place].dest = targetPc/4;

    //first update or clean the state machine
    if (update_table)
    {//update the wanted machine
        history = table.lines[place].hist->current;
        if (table.share == 0)
            machine = history;
        else if (table.share == 1)
            machine = history^(pc_sr2 % (table.lines[place].hist->max));
        else
            machine = history^(int)((pc_sr2 % (table.lines[place].hist->max)) / pow(2,14));

        if (taken)
        {
            if (table.lines[place].states->machines[machine] < 3)
                table.lines[place].states->machines[machine] += 1;
        }
        else
        {
            if (table.lines[place].states->machines[machine] > 0)
                table.lines[place].states->machines[machine] -= 1;
        }
    }
    else
    {//clean all the machines
        for(int j = 0; j < pow(2,table.lines[place].hist->max); j++)
            table.lines[place].states->machines[j] = table.base_state;
    }

    //second update or clean the history
    if (update_hist)
    {//update the history
        table.lines[place].hist->current = table.lines[place].hist->current * 2;//shift left
        if (taken)
            table.lines[place].hist->current += 1;
        table.lines[place].hist->current = (table.lines[place].hist->current) % (table.lines[place].hist->max);
    }
    else
    {//clean it
        table.lines[place].hist->current = 0;
    }

    //updating the stats
    stats.br_num += 1;
    if ((is_taken != taken) || (is_taken && taken && targetPc != pred_dst)) //flush
        stats.flush_num += 1;
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
    *curStats = stats;
    for (int i = 0; i < table.max; i++)
    {
        if (!(table.isGlobalHist) || i == 0)
            free(table.lines[i].hist);
        if (!(table.isGlobalTable) || i == 0)
            free(table.lines[i].states);
    }
	return;
}

