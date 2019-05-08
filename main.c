/* ************************************************************************** */
/** Descriptive File Name
 @Date
    15-May-2018
  
  @Programmer
    Josué Pagán  
   
  @Company
    Universidad Politécnica de Madrid

  @File Name
    main.c

  @Summary
  Activity recognition from acceleration data (analog).
  Node located in the right arm (RA)
  Trained for 5 different activities, as: 
  (#label:activity) --> 18: jumping, 2: standing, 12: running, 10: walkingSlow, 1: sitting 
   
  @Note: 
    josue: if we get this message: A heap is required, but has not been specified... 
    This is due to memory allocation in fsm_new. 
    See: http://www.microchip.com/forums/m737701.aspx
    and see "The heap" in https://os.mbed.com/handbook/Memory-Model
/* ************************************************************************** */

/* INLCUDES */
#include <stdlib.h>     //para max(), min()...
#include <math.h>
#include "system.h"

/* VARIABLES */
volatile int flags = 0x00;

enum fsm_state {
    GET_DATA,    
    COMPUTE_FEATURES,
    COMPUTE_ACTIVITY,
};

/*
 * Maquina de estados: lista de transiciones
 * { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
 */
static fsm_trans_t tracker_tt[] = {
  { GET_DATA, checkSample, GET_DATA, storeData },  
  { GET_DATA, checkBlockSize, COMPUTE_FEATURES, computeFeatures },
  { COMPUTE_FEATURES, checkFeatures, COMPUTE_ACTIVITY, computeActivity },
  { COMPUTE_ACTIVITY, checkActivity, GET_DATA, sleep },
  {-1, NULL, -1, NULL },
};

/* FUNCTIONS */
/* @Description: Interrupt Routine Service
 */
void __ISR(_TIMER_1_VECTOR, IPL5AUTO) timer1_ISR (void) {
    static int storedSamples = 0;
    SLEEP_OFF();
    
    if (storedSamples >= SEGMENT_LENGTH){
        flags |= FLAG_BLOCK_COMPLETED;
        storedSamples = 0; // Restore counter
    }
    else {
        flags |= FLAG_SAMPLE_AVAILABLE;
        storedSamples++;
    }
    TIMER1_IF_CLEAR();
}


// Check functions
int checkSample (fsm_t *this) {
    return (flags & FLAG_SAMPLE_AVAILABLE);
}

int checkBlockSize (fsm_t *this) {
    return (flags & FLAG_BLOCK_COMPLETED);
}

int checkActivity (fsm_t* this) {
    return (flags & FLAG_ACTIVITY_COMPUTED);
}

int checkFeatures (fsm_t *this) {
    return (flags & FLAG_FEATURES_COMPUTED);
}

// Output functions
void storeData (fsm_t *this) {
    tracker *p_tracker;
    int currentSample;
    
    p_tracker = (tracker*)(this->user_data);
    currentSample = p_tracker->sampleIndex;
    p_tracker->accelBlock[currentSample][0] = adc2volts(analogRead(ANALOG_CON2_A1_REG));
    p_tracker->accelBlock[currentSample][1] = adc2volts(analogRead(ANALOG_CON2_A2_REG));
    p_tracker->accelBlock[currentSample][2] = adc2volts(analogRead(ANALOG_CON2_A3_REG));
    p_tracker->sampleIndex++;
    flags &= ~FLAG_SAMPLE_AVAILABLE; // Restore flag
    SLEEP_ON(); IDLE();
}

void computeFeatures (fsm_t *this) {
    tracker *p_tracker;

    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    computeMax(this);
    computeMin(this);
    computeMean(this);
    computeAmp(this);
    computeMed(this);
    computeVar_Std(this);

    p_tracker->sampleIndex = 0; // Restore index
    storeData(this); // Store waiting data in ADC (maybe is not the best option, but it works...)
    flags &= ~FLAG_BLOCK_COMPLETED; // Restore flag
    flags |= FLAG_FEATURES_COMPUTED;
}

void computeActivity (fsm_t *this) {
    int c, f; 
    double distances[NUM_CLUSTERS];
    tracker *p_tracker;

    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    // Compute Euclidean distance to clusters
    for (c = 0; c < NUM_CLUSTERS; c++){
        double partialSum = 0.0;
        for (f = 0; f < NUM_FEATURES; f++){
            double diff = p_tracker->features[f] - p_tracker->classifier.clusterCentrodids[c][f];
            partialSum +=  diff*diff;                        
        }
        distances[c] = sqrt(partialSum);
    }
    
    // Choose the closest cluster
    double closestDistance = distances[0];
        for (c = 0; c < NUM_CLUSTERS; c++){
        if (distances[c] < closestDistance){
            closestDistance = distances[c]; 
            p_tracker->lastActivity = activities[c]; 
        }    
    }    
    if (p_tracker->activitiesDetected >= MAX_NUM_RECORDS){
        p_tracker->activitiesDetected = 0; // Restart: circular buffer
    }
    p_tracker->pastActivities[p_tracker->activitiesDetected++] = p_tracker->lastActivity;
    flags &= ~FLAG_FEATURES_COMPUTED; // Restore flag
    flags |= FLAG_ACTIVITY_COMPUTED;
}

void sleep (fsm_t *this) {
    SLEEP_ON();
    IDLE();
    flags &= ~FLAG_ACTIVITY_COMPUTED;
}

/* MY LOCAL FUNCTIONS */

void computeMax(fsm_t* this){
    int r, c;
    double maxValue[NUM_CHANNELS_ACCEL];
    double myRow[SEGMENT_LENGTH];
    tracker *p_tracker;
    
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for(r = 0; r < SEGMENT_LENGTH; r++){
            myRow[r] = p_tracker->accelBlock[r][c];
            if(maxValue[c] < myRow[r]){
                maxValue[c] = myRow[r];
            }
        }
        p_tracker->features[maxIdx[c]] = maxValue[c];
    }
}

void computeMin(fsm_t* this){
    int r, c;
    double minValue[NUM_CHANNELS_ACCEL];
    double myRow[SEGMENT_LENGTH];
    tracker *p_tracker;
    
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for(r = 0; r < SEGMENT_LENGTH; r++){
            myRow[r] = p_tracker->accelBlock[r][c];
            if(minValue[c] > myRow[r]){
                minValue[c] = myRow[r];
            }
        }
       
        p_tracker->features[minIdx[c]] = minValue[c];
    }
}

void computeMean(fsm_t* this){
    int r, c;
    double suma[NUM_CHANNELS_ACCEL];
    double meanValues[NUM_CHANNELS_ACCEL];
    static int n = SEGMENT_LENGTH;
    tracker *p_tracker;
    
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for(r = 0; r < SEGMENT_LENGTH; r++){
            suma[c] += p_tracker->accelBlock[r][c];
        }
        
        meanValues[c] = suma[c]/n;
        p_tracker->features[meanIdx[c]] = meanValues[c];
    }
}

void computeAmp(fsm_t* this){       //MAX - MEDIA
    int r, c;
    double ampValue[NUM_CHANNELS_ACCEL];
    double maxValue[NUM_CHANNELS_ACCEL];
    double suma[NUM_CHANNELS_ACCEL];
    double meanValues[NUM_CHANNELS_ACCEL];
    double myRow[SEGMENT_LENGTH];
    static int n = SEGMENT_LENGTH;
    tracker *p_tracker;
    
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for(r = 0; r < SEGMENT_LENGTH; r++){
            myRow[r] = p_tracker->accelBlock[r][c];
            if(maxValue[c] > myRow[r]){
                maxValue[c] = myRow[r];
            }
            suma[c] += myRow[r];
        }
        meanValues[c] = suma[c]/n;
        
        ampValue[c] = maxValue[c] - meanValues[c];
        p_tracker->features[ampIdx[c]] = ampValue[c];
    }
}

/* Josue: We must create our own compare function.
 This is an example from Stack Overflow*/
double myCompare (const void* a, const void* b) {
    return ( *(double*)a - *(double*)b );
}

void computeMed(fsm_t* this){
    int r, c;
    tracker *p_tracker;
    double myRow[SEGMENT_LENGTH];
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);

    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for (r = 0; r < SEGMENT_LENGTH; r++) {
            myRow[r] = p_tracker->accelBlock[r][c];
        }
        qsort(myRow, SEGMENT_LENGTH, sizeof(myRow[0]), myCompare); // Short myRow
        p_tracker->features[medIdx[c]] = myRow[SEGMENT_LENGTH/2]; // Value in the middle 
    }
}

void computeVar_Std(fsm_t* this) {
    int r, c;
    double myRow[SEGMENT_LENGTH];
    double numVar[NUM_CHANNELS_ACCEL];      //numerador de la varianza
    double maxValue[NUM_CHANNELS_ACCEL];
    double var[NUM_CHANNELS_ACCEL];
    static double n = SEGMENT_LENGTH;
    tracker *p_tracker;
    
    // Retrieve the user data
    p_tracker = (tracker*)(this->user_data);
    
    for (c = 0; c < NUM_CHANNELS_ACCEL; c++) {
        for (r = 0; r < SEGMENT_LENGTH; r++) {
            if(maxValue[c] < myRow[r]){
                maxValue[c] = myRow[r];
            }
        }
        for (r = 0; r < SEGMENT_LENGTH; r++) {
            numVar[c] += pow((maxValue[c] - myRow[r]), 2);   //pow(max, 2)
        }
        var[c] = numVar[c]/n;
        
        p_tracker->features[varIdx[c]] = var[c];                                //ERROR DEL ENUNCIADO INICIAL: SD -> var(cambiado)
        p_tracker->features[stdIdx[c]] = sqrt(p_tracker->features[varIdx[c]]);  //var = SD^2
    }    
}


int analogRead(char analogPIN){
    ADC1_SELECT_CHANNEL(analogPIN);  // Select the input pins to be connected to the SHA.
    
    /* Ensure Persistent bits are cleared */
    ADC1_CLR_DONE();
    ADC1_IF_CLEAR();  
    
    ADC1_CONVERT();              // Start conversion
   
    while(!ADC1_CKECK_DONE);     // Wait until conversion done

    return ADC1BUF0;      // Result stored in ADC1BUF0
 }

double adc2volts(int adcValue){
    double volts = (VREF_MV* (double)adcValue)/((1<<ADC1_NUM_BITS)-1);    
    return volts;
}

/* INITIALIZATION FUNCTIONS */
void adcManualConfig(int prescale, int tadMultiplier){
    ADC1_OFF();    // disable ADC before configuration
    
    ADC1_AUTOSTART();     // Sampling begins immediately after last conversion completes; SAMP bit is automatically set.
    ADC1_FORM_INT16();    // (Default) Data output format int16 (only 10 bits available) 
    ADC1_VREF_TO_VDD();   // (Default) Set voltage reference to pins AVSS/AVDD
    ADC1_USE_PERIPHERAL_CLOCK(); // (Default) 
    ADC1_CLOCK_PRESCALE(prescale); 
    ADC1_TIME_SAMPLING(tadMultiplier);
    
    ADC1_ON(); // Enable ADC
}

void sensor_setup(){
    ANALOG_CON2_A1_INIT(); // (Default) set RB2 (AN2) to analog. 0 as analog, 1 as digital
    ANALOG_CON2_A1_AS_INPUT(); // (Default) set RB2 as an input 
    ANALOG_CON2_A2_INIT(); // (Default) set RB3 (AN3) to analog. 0 as analog, 1 as digital
    ANALOG_CON2_A2_AS_INPUT(); // (Default) set RB2 as an input 
    ANALOG_CON2_A3_INIT(); // (Default) set RB4 (AN4) to analog. 0 as analog, 1 as digital
    ANALOG_CON2_A3_AS_INPUT(); // (Default) set RB2 as an input 
    adcManualConfig(0, 0); // ADC clock. Max peripheral clock / 512
                           // Acquisition time 0 is not allowed                                         
}

void timer1_setup () {    
    // INT step 2
    TIMER1_PRESCALE_1_256(); // Prescaler 256
    PR1 = 1249; // 40 ms and 256 prescaler
    TIMER1_SOURCE_INTERNAL(); // Internal peripheral clock
    TIMER1_ENABLE();  
    TIMER1_INTERR_PRIOR(5); // INT Step 3
    TIMER1_INTERR_SUBPRIOR(0); 
    TIMER1_RESET(); // INT Step 4
    TIMER1_INTERR_EN(); // INT Step 5   
}

/* Declare my tracker and my classifier
 * - tracker: store acceleration data, features, and detected activities 
 * - classifier: contains the classification model (centroids and activity labels)
 */
void systemSetup(tracker* tracker){
    int i, j;
    classifier classifier;

    for (j = 0; j < NUM_ACTIVITIES; j++){
        for (i = 0; i < NUM_FEATURES; i++){
            classifier.clusterCentrodids[j][i] = clusters[j][i];
        }
    }
    tracker->sampleIndex = 0;
    tracker->activitiesDetected = 0;
    tracker->classifier = classifier;
}

int main() {
    tracker myTracker;
    
    // Initialize my activity recognition system
    systemSetup(&myTracker);

    // Initialize the FSM
    fsm_t* fsm = fsm_new(GET_DATA, tracker_tt, &myTracker); // Sample index as user data info

    timer1_setup();
    sensor_setup(); // Configure ADC

    INTERR_MVEC_EN(); //Interrupt Controller configured for multi vectored mode
    GLOBAL_INTERR_EN(); // Enable global interrupt

    while(1) {
      fsm_fire(fsm);    
    }
    return 0;
}
