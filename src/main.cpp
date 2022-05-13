/**
 * @file main.cpp
 * @author Alan Cohen (al@cobelle.org)
 * @brief
 * @version 0.1
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022
 *
 * Simulates the VolksEEG front end by
 * 1. Reading signals from a waveform file
 * 2. Passing signals out over the serial port, using the VolksEEG simple 
 *    packet format
 * 
 * Some things:
 * a. Only channels that are sampled at the same frequency as the first channel will be used
 * b. Annotation channels are not used
 * c. In adhering to the simple packet spec it will always send 8 samples per packet
 *    -- if fewer than 8 qualifying channels in the EDF file, channels will be padded out
 *    -- if more than 8 qualifying channels, first 8 will be used.
 *  
 */

#include <Arduino.h>
#include <SPI.h>
#include "SdFat.h"
#include "microedf.h"
#include "SimplePacketMaker.h"
#include <Adafruit_TinyUSB.h>
#include "helpers.h"

#define CS_PIN 6 //GPIO output pin for SD card select
#define SEND_PACKET_TEST_PIN 9 //GPIO pin that gets twiddled when packet sent
#define GENERAL_TEST_PIN_1 10 //GPIO pin that gets twiddled for testing
#define GENERAL_TEST_PIN_2 11 //GPIO pin that gets twiddled for testing
#define SD_INFO_DUMP false //true if we want to send SD card filesystem data to serial out
#define GPIO_DEBUG true //if true, various GPIOs are toggled to indicate points reached in code

unsigned long long GetCorrectedMicros();
void Dump_sd_info(SdFat sd);
void GetEdfField(int startOffset, int fieldLength);
void GetAllEdfFields(); //reads values of first 
void CreateOutArray();
void RefillBuffer();
void WriteNextSamples();
void WriteNextPacket();
void WritePackets();
void InvertPin(uint32_t pinNum);

FatFile edfFile;
bool sdInitialized = false;

edfHeaderMain *fileHeader; // holds the EDF file's header struct
int fileHeaderLength;

short **outArray;
int numOutArrayRows;

float outSamples[8];
unsigned long nextMicros; //would prefer to make this a uint32_t to match micros() return type, but print/println won't accept that type
unsigned long numPacketsWritten = 0; //would prefer to make this a uint32_t to match micros() return type, but print/println won't accept that type

/*
 * Holds the first channel's sampling period.
 * We'll only use channels with the same sampling period.
 * This keeps code simple.
 */
double acceptedSamplingPeriodMicros;
// Array of lenghth = number of channels
// Values set to true if that channel's sampling rate = accepted sampling rate
bool *isAcceptableSamplingFreq;

chanAttributes *chanAttr;

/* holds the raw channel headers block from the edf file
   which is later parsed into chanHeaders array */
char *chanHeadersRaw;
int chanHeadersRawLength;

// an array holding the channel header structs
edfHeaderChan *chanHeaders; 

int numChans;

/* points to same memory address as chanHeaders, allows us to access as
a char array instead of as array of edfHeaderChans */
char *chanHeadersChars;
bool isOutputting = true;

void setup()
{
  Serial1.begin(115200, SERIAL_8N1);
  SdFat sd;

  pinMode(CS_PIN, OUTPUT); // SD card select
  if (GPIO_DEBUG)
  {
    pinMode(SEND_PACKET_TEST_PIN, OUTPUT);
    pinMode(GENERAL_TEST_PIN_1, OUTPUT);
    pinMode(GENERAL_TEST_PIN_2, OUTPUT);
    digitalWrite(SEND_PACKET_TEST_PIN, LOW);
  }

  if (!sd.begin(CS_PIN))
  {
    Serial1.println("SD card initialization failed!");
    return;
  }

  if (SD_INFO_DUMP)
  {
    Dump_sd_info(sd);
  }

  // open the file for reading:
  delay(1000);
  edfFile = sd.open("output.edf", FILE_READ);
  if (edfFile.isOpen())
  {
    edfFile.rewind(); //goes to beginning of file
    
    //Read EDF file header
    fileHeaderLength = sizeof(edfHeaderMain);
    fileHeader = (edfHeaderMain *)malloc(fileHeaderLength);
    edfFile.read(fileHeader, fileHeaderLength);

    // create an array of channel headers
    numChans = ((String)fileHeader->numSignals).toInt();
    chanHeaders = new edfHeaderChan[numChans];
    chanHeadersRawLength = sizeof(edfHeaderChan) * numChans; // holds the entire set of channel headers as read from the edf file
    chanHeadersRaw = (char *)malloc(chanHeadersRawLength);
    edfFile.read(chanHeadersRaw, chanHeadersRawLength);
    chanHeadersChars = (char *)chanHeaders;
    GetAllEdfFields(); //reads EDF heasder files into channel structures
    isAcceptableSamplingFreq = new bool[numChans];
    free(chanHeadersRaw);

    //Gets the samples/record for the first channel
    //only channels with the same samples/second will be used for output
    const int acceptedSampsPerRecord = ((String)chanHeaders[0].numDataSamplesPerRecord).toInt();
    float recsPerSec = ((String)fileHeader->durationDataRcordsSecs).toFloat();
    float acceptedSamplingPeriodSecs = 1.0/(acceptedSampsPerRecord * recsPerSec);
    acceptedSamplingPeriodMicros = acceptedSamplingPeriodSecs * 1000000;
    //acceptedSamplingPeriodMicros = 5000;
    //isAcceptableSamplingFreq = new bool[numChans];
    chanAttr = new chanAttributes[numChans];
    chanAttr[0].isAcceptableSamplingFreq = true;

    //get more attributes for each channel (beyond what's in header)
    for (int i = 0; i < numChans; i++)
    {
      //Check each channel to see if samples/second is acceptable
      int thisSampsPerRecord = ((String)chanHeaders[i].numDataSamplesPerRecord).toInt();
      if (thisSampsPerRecord == acceptedSampsPerRecord)
      {
        chanAttr[i].isAcceptableSamplingFreq = true;
      }
      else
      {
        chanAttr[i].isAcceptableSamplingFreq = false;
      }

      //calibrate each channel
      //first the multiplier
      int digitalMin = ((String)chanHeaders[i].digitalMin).toInt();
      int digitalMax = ((String)chanHeaders[i].digitalMax).toInt();
      float physicalMin = CharArrayToFloat(chanHeaders[i].physicalMin, sizeof(chanHeaders[i].physicalMin));
      float physicalMax = CharArrayToFloat(chanHeaders[i].physicalMax, sizeof(chanHeaders[i].physicalMax));

      chanAttr[i].calMultiplier = (physicalMax - physicalMin)/(digitalMax - digitalMin);
      chanAttr[i].calOffset = physicalMin - (chanAttr[i].calMultiplier * digitalMin);
    }
    numOutArrayRows = ((String)chanHeaders[0].numDataSamplesPerRecord).toInt();
    CreateOutArray();

    // populate outArray
    RefillBuffer();
    edfFile.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial1.println("error opening output.edf");
  }
  nextMicros = micros();
}

void loop()
{
  if (isOutputting)
  {
    unsigned long long nowMicros = GetCorrectedMicros();
    InvertPin(GENERAL_TEST_PIN_1);
    if (nowMicros > nextMicros) 
    {
      if (GPIO_DEBUG)
      {
        digitalWrite(GENERAL_TEST_PIN_2, HIGH);
      }
      WriteNextPacket();
      nextMicros += acceptedSamplingPeriodMicros;
      if (GPIO_DEBUG)
      {
        digitalWrite(GENERAL_TEST_PIN_2, LOW);
      }
      unsigned long nowSecs = nowMicros / 1000000;
    }
  }
  else
  {
    //check if we've received anything
    if (Serial1.available() > 0)
    {
      isOutputting = true;
    }
  }
}

unsigned long long GetCorrectedMicros()
{
  static unsigned long long previousAccumulatedMicros = 0;
  static unsigned long long previousRawMicros = 0;
  static unsigned long long accumulatedMicros = 0;
  unsigned long long currentRawMicros = micros();
  //uint32_t sincePrevMicros;
  if (accumulatedMicros > 4294000000)
  {
    Serial1.print("");
  }
  if (currentRawMicros >= previousRawMicros)
  {
    //sincePrevMicros = currentMicros - previousAccumulatedMicros;
    accumulatedMicros += (currentRawMicros - previousRawMicros);
  }
  else
  {
    unsigned long long adjustedRaw = currentRawMicros + pow(2,26);
    unsigned long long delta = adjustedRaw - previousRawMicros;
    adjustedRaw = adjustedRaw % (unsigned long long)pow(2,26);
    adjustedRaw = currentRawMicros;
  }
  previousRawMicros = currentRawMicros;
  previousAccumulatedMicros = accumulatedMicros;
  return accumulatedMicros;
}

void InvertPin(uint32_t pinNum)
{
  if (GPIO_DEBUG)
  {
    bool currVal = digitalRead(pinNum);
    digitalWrite(pinNum, !currVal);
  }
}

void WriteNextSamples()
{

}

void WriteNextPacket()
{
  bool currentTestPinVal = digitalRead(SEND_PACKET_TEST_PIN);
  if (GPIO_DEBUG)
  {
    digitalWrite(SEND_PACKET_TEST_PIN, !currentTestPinVal);
  }
  OutPacket outPacket;
  unsigned long rowInBuffer = numPacketsWritten % numOutArrayRows;
  outPacket.counter = numPacketsWritten % 32768; //32769 = 2e15;
  for (int chan = 0; chan < numChans; chan++)
  {
    if (chan < 8)
    {
      int32_t digitalOut = outArray[chan][rowInBuffer];
      float physOut = (digitalOut * chanAttr[chan].calMultiplier) + chanAttr[chan].calOffset;
      outPacket.values[chan] = (int32_t)physOut;
//      if (chan == 4)
//      {
//        Serial1.print(physOut);
//      }
    }
  }
  SendOutPacket(&outPacket); 
  numPacketsWritten++;
  if (rowInBuffer == (numOutArrayRows - 1))
  {
    RefillBuffer();
  }
}

void WritePackets()
{
  OutPacket outPacket;
  for (int row = 0; row < numOutArrayRows; row++)
  {
    for (int chan = 0; chan < numChans; chan++)
    {
      if (chan < 8)
      {
        int32_t digitalOut = outArray[chan][row];
        float physOut = (digitalOut * chanAttr[chan].calMultiplier) + chanAttr[chan].calOffset;
        outPacket.values[chan] = (int32_t)physOut;
      }
    }
    SendOutPacket(&outPacket); 
  }
}



void RefillBuffer()
{
  for (int col = 0; col < numChans; col++)
  {
    if (isAcceptableSamplingFreq[col])
    {
      //read the values for the next column in the record
      char *ptrOutCol = (char *)(&(outArray[col][0]));
      edfFile.read(ptrOutCol, numOutArrayRows * 2); // * 2 because two bytes per sample

      // commented out code below checks what's coming in on one column
      //if (col == 1)
      /*
      {
        Serial1.println(outArray[col][10]);
        Serial1.println("----------------------------------");
        Serial1.println("----------------------------------");
        chanHeaders[col].label[15] = 0;
        String chanLabel = chanHeaders[col].label;
        Serial1.println(chanLabel);
        Serial1.println("----------------------------------");
        Serial1.println("----------------------------------");
        Serial1.println(col);
        for (int i = 0; i < numOutArrayRows; i++)
        {
          Serial1.println(outArray[col][i]);
        }
      }
      */
    }
    else
    {
      int sampsThisRecord = ((String)chanHeaders[col].numDataSamplesPerRecord).toInt();
      edfFile.seekCur(sampsThisRecord * 2);
    }
  }
}

void GetAllEdfFields()
{
  GetEdfField(0, 16);
  GetEdfField(16, 80);
  GetEdfField(96, 8);
  GetEdfField(104, 8);
  GetEdfField(112, 8);
  GetEdfField(120, 8);
  GetEdfField(128, 8);
  GetEdfField(136, 80);
  GetEdfField(216, 8);
  GetEdfField(224, 32);
}

/**
 * @brief Creates the 2D Output Array
 * 
 * 
 */
void CreateOutArray()
{
  // Create the 2D output array as a one-record buffer
  outArray = new short *[numChans];
  for (int chan = 0; chan < numChans; chan++)
  {
    outArray[chan] = new short[numOutArrayRows]; // fix at 8 columns
  }

  // populate array with "fake data" - will be overwritten by real data 
  // unless the channel isn't used
  // all values in channel 0 are 0, all values in channel 1 are 1, etc
  for (int col = 0; col < numChans; col++)
  {
    for (int row = 0; row < numOutArrayRows; row++)
    {
      if (col < 8)
      {
        outArray[col][row] = col;
      }
    }
  }
}

/**
 * @brief Given the start amd length of the field within the header
 *        reads that field into the correct struct field for each channel
 *        
 * @param startOffset 
 * @param fieldLength 
 */
void GetEdfField(int startOffset, int fieldLength)
{
  //char tempHolder[fieldLength];
  int chanHeaderLength = sizeof(chanHeaders[0]); //chanHeaders[] is a global variable
  for (int i = 0; i < numChans; i++)
  {
    int offsetInCharArray = (startOffset * numChans) + (i * fieldLength);
    for (int j = 0; j < fieldLength; j++)
    {
      //grab the bytes
      //tempHolder[j] = chanHeadersRaw[j + offsetInCharArray];
      chanHeadersChars[j + (chanHeaderLength * i) + startOffset] = (chanHeadersRaw[j + offsetInCharArray]);
    }
  }
  return;
}

void Dump_sd_info(SdFat sd)
{
  Serial1.print("Clusters:          ");
  Serial1.println(sd.clusterCount());
  Serial1.print("Blocks x Cluster:  ");
  Serial1.println(sd.blocksPerCluster());
  Serial1.print("Total Blocks:      ");
  Serial1.println(sd.blocksPerCluster() * sd.clusterCount());
  Serial1.println();
  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial1.print("Volume type is:    FAT");
  Serial1.println(sd.fatType(), DEC);
  volumesize = sd.blocksPerCluster(); // clusters are collections of blocks
  volumesize *= sd.clusterCount();    // we'll have a lot of clusters
  volumesize /= 2;                    // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial1.print("Volume size (Kb):  ");
  Serial1.println(volumesize);
  Serial1.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial1.println(volumesize);
  Serial1.print("Volume size (Gb):  ");
  Serial1.println((float)volumesize / 1024.0);

  // Note - sd.ls prints to Serial, not Serial1 -- not sure if can call
  // differently to work with Serial1
  Serial1.println("\nFiles found on the card (name, date and size in bytes): ");
  sd.ls(LS_R | LS_DATE | LS_SIZE);
  Serial1.println(" ");
}