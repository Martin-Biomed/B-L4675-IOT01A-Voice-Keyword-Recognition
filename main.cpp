#include "mbed.h"
#include "model-parameters/model_metadata.h"
#include "ei_run_classifier.h"
#include "model_metadata.h"
#include "stm32l475e_iot01_audio.h"
// #include "ei_microphone.h"
#include "ei_microphone_inference.h"

#include <string.h> 

#include "rtos/Mutex.h"
#include "rtos.h"
#include "numpy.hpp"

/// Had to modify model-parameters/model_metadata.h because buffers were overflown (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW used to be 4)

//rtos::Thread thread1;

// Settings
#define DEBUG 0                 // Enable pin pulse during ISR  
enum { ADC_BUF_LEN = 1600 };    // Size of one of the DMA double buffers

// Audio buffers, pointers and selectors
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

// Globals - Edge Impulse
static inference_t inference;
static bool record_ready = false;
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

// Section added for use with the functions from: https://github.com/edgeimpulse/firmware-st-b-l475e-iot01a/blob/master/source/main.cpp

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t mic_params;

static bool debug_option = 0;

// Variables used to print a single output

float biggest_value = 0; // Needed to declare variables here so I could jump between labels
static char prediction[8] = {0};  // Stores the value of the prediction
static char noise[8] = {0};  // Stores the char formatted version of _noise
int string_int = 0;  // sprintf: If successful, it returns the total number of chars written excluding null-character


bool ei_microphone_init(void) {

    mic_params.BitsPerSample = 16;
    mic_params.ChannelsNbr = AUDIO_CHANNELS;
    mic_params.Device = AUDIO_IN_DIGITAL_MIC1;
    mic_params.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    mic_params.Volume = 32;

    printf("Microphone initialized\n");

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &mic_params);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Init (%d)\r\n", ret);
        return false;
    }
    return true;
}


int main(void)
{

    printf("Microphone parameters initialized\n");

    bool init_success = ei_microphone_init();

    printf("\n\nHello from the Edge Impulse Device SDK.\n");

    if (EI_CLASSIFIER_FREQUENCY != 16000) {
        printf("ERR: Frequency is %d but can only sample at 16000Hz\n", (int)EI_CLASSIFIER_FREQUENCY);
        //return;
    }

    // summary of inferencing settings (from model_metadata.h)
    printf("Inferencing settings:\n");
    printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    printf("\n");

    //ei_microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE);

    // Set inference parameters
    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = EI_CLASSIFIER_SLICE_SIZE;
    inference.buf_ready = 0;

    // Create double buffer for inference
    ei_microphone_inference_start(inference.n_samples);


    while (1) {

        //printf("Entered infinite loop\n");

        biggest_value = 0;

        // ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
        bool m = ei_microphone_inference_record();
   
        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &ei_microphone_audio_signal_get_data;
        ei_impulse_result_t result = { 0 };

        //printf("Debug Option: %d\n", debug_option);

        EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_option);
        //printf("run_classifier returned: %d\n", r);
        if (r != EI_IMPULSE_OK) {
            printf("ERR: Failed to run classifier (%d)\n", r);
            break;
        }


       //if(++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW >> 1)) {
        // print the predictions
        //printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
        //    result.timing.dsp, result.timing.classification, result.timing.anomaly);
        
        string_int = sprintf(noise, result.classification[0].label);
        //printf("First Label: %s \n", noise);  

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            //printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);

            if (result.classification[ix].value >= biggest_value){
                biggest_value = result.classification[ix].value;
                string_int = sprintf(prediction, result.classification[ix].label);        
            }
        }

        if (strcmp(prediction, noise) != 0){
            if (biggest_value >= 0.6){  // The degree of certainty must be greater than 40%, otherwise it could be a floating prediction
                printf("Predicted Keyword: %s \n", prediction);
            }
        }
        //#if EI_CLASSIFIER_HAS_ANOMALY == 1
        //printf("    anomaly score: %.3f\n", result.anomaly);
        //#endif

        print_results = 0;

        //ThisThread::sleep_for(10ms); //Sleep for 2000 ms
            //}
        }   

    ei_microphone_inference_end();

    //}

}
