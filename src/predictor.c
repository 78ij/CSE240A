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
uint32_t g_history_reg = 0;
uint64_t perceptron_history_reg = 0;
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
char *choice_table;
uint32_t *local_history_table;
char *local_prediction;

// Our custom predictor
// use int8 as perceptron weights
// assume the history bit is x:
// the space usage is (x + (2^x) * x * 8) bits
// assuming x = 10, we have space = 8 bits + 10KB
// assuming x = 12, we have space = 20 bits + 48KB
// 12 is the highest.
int8_t *perceptron_table;

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
    {
      // global prediction table
      size_t total_entries_for_predictor= 1 << ghistoryBits;
      g_predictors = calloc(total_entries_for_predictor,1);
      // set all the predictors to weakly not taken (01)
      memset(g_predictors, 1, total_entries_for_predictor);

      // choice table
      choice_table = calloc(total_entries_for_predictor,1);
      // set all the choices to weakly select global (01)
      // 00: strong global
      //01: weak global
      //10: weak local
      //11: strong local
      memset(choice_table, 1, total_entries_for_predictor);

      // local history table
      size_t total_entries_for_local_history_table= 1 << pcIndexBits;
      local_history_table = calloc(total_entries_for_local_history_table,sizeof(uint32_t));

      // local prediction table
      size_t total_entries_for_local_predictor = 1 << lhistoryBits;
      local_prediction = calloc(total_entries_for_local_predictor,1);
      
      break;
    }
    case CUSTOM:
    {
      ghistoryBits = 59;
      size_t perceptron_entry_number = 1024;
      // table of (ghistorybits) int8.
      perceptron_table = calloc(perceptron_entry_number, ghistoryBits + 1);
      break;
    }
      
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
    case TOURNAMENT:{
      uint32_t mask = 1 << ghistoryBits;
      mask = mask - 1;
      uint32_t pc_lower = pc & mask;
      uint32_t history_lower = g_history_reg & mask;
      uint32_t entry_address = history_lower ^ pc_lower;
      uint8_t predict_global = get_predict(g_predictors + entry_address);

      uint32_t mask_local = 1 << pcIndexBits;
      mask_local = mask_local - 1;
      uint32_t pc_lower_local = pc & mask_local;

      uint32_t mask_localhistory = 1 << lhistoryBits;
      mask_localhistory = mask_localhistory - 1;

      uint32_t local_history = *(local_history_table + pc_lower_local);
      uint8_t predict_local = get_predict(local_prediction + (local_history & mask_localhistory));
      uint8_t choice = get_predict(choice_table + pc_lower);
      // 0: global
      // 1: local
      if (choice) return predict_local;
      else return predict_global;
      break;
    }
    case CUSTOM:
      {
        uint32_t mask = 1 << 10;
        mask = mask - 1;
        uint32_t pc_lower = pc & mask;
        uint32_t history_lower = ((uint32_t) (perceptron_history_reg & 0xFFFFFFFF) & mask) ;
        uint32_t entry_address = history_lower ^ pc_lower;
        int8_t *perceptron = perceptron_table + (pc_lower * (ghistoryBits + 1));
        // calculate perceptron result
        // from low to high
        int32_t result = *perceptron;
        for(int i = 0; i < ghistoryBits; i++){
          uint64_t mask_lsb = 1 << i;
          // in the perceptron setting, not taken is -1, not 0
          if (mask_lsb & perceptron_history_reg){
            result += *(perceptron + i + 1);
          }
          else{
            result -= *(perceptron + i + 1);
          }
        }
        if(result >= 0) return 1;
        else return 0;
        break;
      }
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
      change_2bit_predictor(g_predictors + entry_address,outcome);
      g_history_reg = g_history_reg << 1;
      if(outcome) g_history_reg = g_history_reg | 1;
      break;
    }
    case TOURNAMENT:{

     uint32_t mask = 1 << ghistoryBits;
      mask = mask - 1;
      uint32_t pc_lower = pc & mask;
      uint32_t history_lower = g_history_reg & mask;
      uint32_t entry_address = history_lower ^ pc_lower;
      uint8_t predict_global = get_predict(g_predictors + entry_address);
      
      uint32_t mask_local = 1 << pcIndexBits;
      mask_local = mask_local - 1;
      uint32_t pc_lower_local = pc & mask_local;

      uint32_t mask_localhistory = 1 << lhistoryBits;
      mask_localhistory = mask_localhistory - 1;

      uint32_t local_history = *(local_history_table + pc_lower_local);
      uint8_t predict_local = get_predict(local_prediction + (local_history & mask_localhistory));
      uint8_t choice = get_predict(choice_table + pc_lower);
      change_2bit_predictor(g_predictors + entry_address, outcome);
      change_2bit_predictor(local_prediction + (local_history & mask_localhistory), outcome);
      if (outcome == predict_local && outcome == predict_global){
        // None
      }
      if (outcome != predict_local && outcome == predict_global){
        // Decrease choice
        change_2bit_predictor(choice_table + pc_lower, 0);
      }
      if (outcome == predict_local && outcome != predict_global){
        //Increase choice
        change_2bit_predictor(choice_table + pc_lower, 1);
      }
      if (outcome != predict_local && outcome != predict_global){
        //None
      }
      g_history_reg = g_history_reg << 1;
      if(outcome) g_history_reg = g_history_reg | 1;

      *(local_history_table + pc_lower_local) = *(local_history_table + pc_lower_local) << 1;
      if(outcome) *(local_history_table + pc_lower_local) = *(local_history_table + pc_lower_local) | 1;
      break;
    }
    case CUSTOM:{
        uint32_t mask = 1 << 10;
        mask = mask - 1;
        uint32_t pc_lower = pc & mask;
        uint32_t history_lower = ((uint32_t) (perceptron_history_reg & 0xFFFFFFFF) & mask) ;
        uint32_t entry_address = history_lower ^ pc_lower;
        int8_t *perceptron = perceptron_table + (pc_lower * (ghistoryBits + 1));
        // calculate perceptron result
        // from low to high
        int32_t result = *perceptron;
        for(int i = 0; i < ghistoryBits; i++){
          uint64_t mask_lsb = 1 << i;
          // in the perceptron setting, not taken is -1, not 0
          if (mask_lsb & perceptron_history_reg){
            result += *(perceptron + i + 1);
          }
          else{
            result -= *(perceptron + i + 1);
          }
        }
        if (outcome && result >= 0) {
          perceptron_history_reg = perceptron_history_reg << 1;
          if(outcome) perceptron_history_reg = perceptron_history_reg | 1;
          return;
        }
        if (!outcome && result < 0  ) {
          perceptron_history_reg = perceptron_history_reg << 1;
          if(outcome) perceptron_history_reg = perceptron_history_reg | 1;
          return;
        }
        if (result >= 126 || result <= -126){
          perceptron_history_reg = perceptron_history_reg << 1;
          if(outcome) perceptron_history_reg = perceptron_history_reg | 1;
          return;
        }

        int8_t update_multiplier = 0;
        if (outcome) update_multiplier = 1;
        else update_multiplier = -1;
        
        *perceptron += update_multiplier;

        for(int i = 0; i < ghistoryBits; i++){
          uint64_t mask_lsb = 1 << i;
          // in the perceptron setting, not taken is -1, not 0
          if (mask_lsb & perceptron_history_reg){
            *(perceptron + i + 1) += update_multiplier;
          }
          else{
            *(perceptron + i + 1) -= update_multiplier;
          }
        }
        perceptron_history_reg = perceptron_history_reg << 1;
        if(outcome) perceptron_history_reg = perceptron_history_reg | 1;  
      break;
    }
      
    default:
      break;
 }
}
