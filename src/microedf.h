#include <stdbool.h>
#define EDFLIB_MAXSIGNALS                (640)
struct edf_hdr_struct{            /* this structure contains all the relevant EDF header info and will be filled when calling the function edf_open_file_readonly() */
  int       handle;               /* a handle (identifier) used to distinguish the different files */
  int       filetype;             /* 0: EDF, 1: EDF+, 2: BDF, 3: BDF+, a negative number means an error */
  int       edfsignals;           /* number of EDF signals in the file, annotation channels are NOT included */
  long long file_duration;        /* duration of the file expressed in units of 100 nanoSeconds */
  int       startdate_day;
  int       startdate_month;
  int       startdate_year;
  long long starttime_subsecond;  /* starttime offset expressed in units of 100 nanoSeconds. Is always less than 10000000 (one second). Only used by EDF+ and BDF+ */
  int       starttime_second;
  int       starttime_minute;
  int       starttime_hour;
  char      patient[81];                                  /* null-terminated string, contains patientfield of header, is always empty when filetype is EDFPLUS or BDFPLUS */
  char      recording[81];                                /* null-terminated string, contains recordingfield of header, is always empty when filetype is EDFPLUS or BDFPLUS */
  char      patientcode[81];                              /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      gender[16];                                   /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      birthdate[16];                                /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      patient_name[81];                             /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      patient_additional[81];                       /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      admincode[81];                                /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      technician[81];                               /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      equipment[81];                                /* null-terminated string, is always empty when filetype is EDF or BDF */
  char      recording_additional[81];                     /* null-terminated string, is always empty when filetype is EDF or BDF */
  long long datarecord_duration;                          /* duration of a datarecord expressed in units of 100 nanoSeconds */
  long long datarecords_in_file;                          /* number of datarecords in the file */
  long long annotations_in_file;                          /* number of annotations in the file */
  //struct edf_param_struct signalparam[EDFLIB_MAXSIGNALS]; /* array of structs which contain the relevant signal parameters */
};

struct edfHeaderMain
{
  char FormatVersion[8];
  char localPatientId[80];
  char localRecordingId[80];
  char startDate[8];
  char startTime[8];
  char numHeaderBytes[8];
  char reserved[44];
  char numDataRecords[8];
  char durationDataRcordsSecs[8];
  char numSignals[4];
};

struct edfHeaderChan
{
  char label[16];
  char transducerType[80];
  char physicalDimension[8];
  char physicalMin[8];
  char physicalMax[8];
  char digitalMin[8];
  char digitalMax[8];
  char preFiltering[80];
  char numDataSamplesPerRecord[8];
  char reserved[32];
};

struct chanAttributes
{
  bool  isAcceptableSamplingFreq;
  float calMultiplier;
  float calOffset;
};