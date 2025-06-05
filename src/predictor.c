//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <string.h>
//
// TODO:Student Information
//
const char *studentName = "JIAMU SUN, ZILU LI";
const char *studentID   = "A69034102, ";
const char *email       = "jis081@ucsd.edu, ";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

// acutal gshare history register,  since pc is uint32_t, it is uint32_t.
uint32_t g_history_reg;
// actual gshare 2-bit predictor.
//         NT      NT      NT
//      ----->  ----->  ----->
//    ST      WT      WN      SN
//      <-----  <-----  <-----
//        T       T       T
// ST: 11, WT: 10, WN:01, SN: 00
// inited to WN.
// Note that we use a whole byte for every 2-bit predictor
// to simplify the implementation.
// actual 
char *g_predictors;

//Used in both gshare and tournament
// since they both use 2-bit predictor.
void change_2bit_predictor(char *g_predictor, uint8_t outcome){
  if(outcome){
    // branch is taken
    if(*g_predictor < 3){
        *g_predictor += 1;
    }
  }
  else{
    //branch is not taken
    if(*g_predictor > 0){
      *g_predictor -= 1;
    }
  }
}

uint8_t get_predict(char *g_predictor){
  if (*g_predictor > 1) return 1;
  else return 0;
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:{
      // we have the total entry of predictions same to (2^ghistory bits)
      size_t total_entries_for_predictor= 1 << ghistoryBits;
      g_predictors = calloc(total_entries_for_predictor,1);
      // set all the predictors to weakly not taken (01)
      memset(g_predictors, 1, total_entries_for_predictor);
      break;
    }
    case TOURNAMENT:
      break;
    case CUSTOM:
      break;
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:{
      // calculate the 'address' of the entry of the prediction table
      // XOR the pc lower bit with the history bit
      // Get a mask based on ghistorybits
      uint32_t mask = 1 << ghistoryBits;
      mask = mask - 1;
      uint32_t pc_lower = pc & mask;
      uint32_t history_lower = g_history_reg & mask;
      uint32_t entry_address = history_lower ^ pc_lower;
      return get_predict(g_predictors + entry_address);
      break;
    }
    case TOURNAMENT:
      break;
    case CUSTOM:
      break;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
 switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
    {
      // calculate the 'address' of the entry of the prediction table
      // XOR the pc lower bit with the history bit
      // Get a mask based on ghistorybits
      uint32_t mask = 1 << ghistoryBits;
      mask = mask - 1;
      uint32_t pc_lower = pc & mask;
      uint32_t history_lower = g_history_reg & mask;
      uint32_t entry_address = history_lower ^ pc_lower;
      return change_2bit_predictor(g_predictors + entry_address,outcome);
      break;
    }
    case TOURNAMENT:
      break;
    case CUSTOM:
      break;
    default:
      break;
 }
}
