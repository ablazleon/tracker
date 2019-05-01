/* ************************************************************************** */
/** Descriptive File Name

  @Date
    24-April-2018
  
  @Programmer
    Josué Pagán  
   
  @Company
    Universidad Politécnica de Madrid

  @File Name

    thermostat.h
  @Description
    Define prototypes of functions and other elements.
 */
/* ************************************************************************** */

#ifndef _SYSTEM_H    /* Guard against multiple inclusion */
#define _SYSTEM_H
#include <xc.h>
#include <sys/attribs.h>
#include <stdlib.h>
#include <math.h>

#include "myMacros_pic32mx440f256h.h"
#include "pinguinoConf.h"
#include "fsm.h"
//#include "featureExtraction.h"


/* DEFINES */
#define FLAG_SAMPLE_AVAILABLE 0x01
#define FLAG_BLOCK_COMPLETED 0x02
#define FLAG_FEATURES_COMPUTED 0x04
#define FLAG_ACTIVITY_COMPUTED 0x08

#define SAMPLING_PERIOD_SEC 60 // Seconds

#define SAMPLING_FREQ_ACCEL 25 // Hz
#define BLOCK_SIZE 5 // seconds
#define SEGMENT_LENGTH BLOCK_SIZE*SAMPLING_FREQ_ACCEL
#define NUM_CHANNELS_ACCEL 3
#define NUM_VARIABLES 7
#define NUM_FEATURES NUM_VARIABLES*NUM_CHANNELS_ACCEL

#define NUM_ACTIVITIES 5
#define NUM_CLUSTERS NUM_ACTIVITIES
#define MAX_NUM_CHAR_NAME 20
#define MAX_NUM_RECORDS 300

/* Variables */double clusters[NUM_CLUSTERS][NUM_FEATURES] = {{1033.3994, 2963.5932, 1398.9645, 2883.6755, 2407.2201, 1807.1426, 2914.5899, 2784.8815, 1940.0148, 3947.9893, 5748.4747, 3338.9793, 2351.7713, 1342.9905, 1032.7974, 103105.5275, 1367448.8717, 341930.4456, 313.39980, 1159.5234, 541.67120},
{9.6494000, 13.577700, 14.514200, 3698.5989, 2199.5764, 1925.7902, 3698.8061, 2199.5210, 1926.0202, 3708.4555, 2213.0987, 1940.5344, 3688.7423, 2185.5653, 1912.1864, 15.424300, 35.019700, 43.383300, 3.7468000, 5.4585000, 5.9532000},
{1127.9750, 2085.6824, 1268.1390, 2549.6278, 2644.4657, 1883.0363, 2600.0308, 2847.4893, 1927.4021, 3728.0059, 4933.1717, 3195.5410, 1480.3745, 1414.8620, 1141.9092, 319543.518, 1002767.4619, 177765.24440, 559.80740, 988.36660, 413.78230},
{232.67480, 343.90060, 256.23070, 3222.2455, 2504.2592, 2312.4564, 3210.5594, 2511.7062, 2313.5920, 3443.2342, 2855.6068, 2569.8227, 2923.6838, 2251.1521, 2066.7505, 12988.0357, 14966.7865, 15445.0516, 96.191100, 108.64980, 103.28840},
{7.4089000, 7.5321000, 5.5204000, 2814.1088, 2512.9521, 2462.8543, 2814.4386, 2513.0772, 2462.7918, 2821.8475, 2520.6093, 2468.3122, 2807.8364, 2505.6207, 2456.9892, 9.2511000,	9.9543000, 6.724, 2.959, 2.8986000, 2.4087000}};

int activities[NUM_ACTIVITIES] = {18, 2, 12, 10, 1};  // 18: jumping, 2: standing, 12: running, 10: walkingSlow, 1: sitting 

int ampIdx[NUM_CHANNELS_ACCEL] = {0, 1, 2};  
int medIdx[NUM_CHANNELS_ACCEL] = {3, 4, 5};  
int meanIdx[NUM_CHANNELS_ACCEL] = {6, 7, 8};  
int maxIdx[NUM_CHANNELS_ACCEL] = {9, 10, 11};  
int minIdx[NUM_CHANNELS_ACCEL] = {12, 13, 14};  
int varIdx[NUM_CHANNELS_ACCEL] = {15, 16, 17};  
int stdIdx[NUM_CHANNELS_ACCEL] = {18, 19, 20};  

typedef struct {
    double clusterCentrodids[NUM_CLUSTERS][NUM_FEATURES];
    char names[NUM_ACTIVITIES][MAX_NUM_CHAR_NAME];
} classifier;

typedef struct {
    int sampleIndex;
    int activitiesDetected;
    int lastActivity;  
    double accelBlock[SEGMENT_LENGTH][NUM_CHANNELS_ACCEL];
    double features[NUM_FEATURES];
    int pastActivities[MAX_NUM_RECORDS];    

    classifier classifier;
    
} tracker;


/* PROTOTYPES FUNCTIONS */
/* Configuration functions */
void led_setup(void);
void timer1_setup(void);
void sensor_setup(void);

/* Check functions*/
int checkSample (fsm_t *this);
int checkBlockSize (fsm_t *this);
int checkFeatures (fsm_t *this);
int checkActivity (fsm_t *this);

/* Actuation functions */
void storeData (fsm_t *this);
void computeFeatures (fsm_t *this);
void computeActivity (fsm_t *this);
void sleep(fsm_t* this);

/* Others */
void adcManualConfig(int prescale, int tadMultiplier);
int analogRead(char analogPIN);
double adc2volts(int adcValue);


/* PROTOTYPES FUNCTIONS */
void computeMax(fsm_t * this);
void computeMin(fsm_t * this);
void computeMean(fsm_t * this);
void computeAmp(fsm_t * this);
void computeMed(fsm_t * this);
void computeVar_Std(fsm_t * this);

#endif /* _SYSTEM_H */