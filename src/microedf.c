#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "microedf.h"

//this is the structure we initially read the header into
struct edfhdrblock
{
    ///FILE      *file_hdl;
    //char      path[1024];
    //int       writemode;
    char      version[32];
    char      patient[81];
    char      recording[81];
    char      plus_patientcode[81];
    char      plus_gender[16];
    char      plus_birthdate[16];
    char      plus_patient_name[81];
    char      plus_patient_additional[81];
    char      plus_startdate[16];
    char      plus_admincode[81];
    char      plus_technician[81];
    char      plus_equipment[81];
    char      plus_recording_additional[81];
    long long l_starttime;
    int       startdate_day;
    int       startdate_month;
    int       startdate_year;
    int       starttime_second;
    int       starttime_minute;
    int       starttime_hour;
    char      reserved[45];
    int       hdrsize;
    int       edfsignals;
    long long datarecords;
    int       recordsize;
    int       annot_ch[EDFLIB_MAXSIGNALS];
    int       nr_annot_chns;
    int       mapped_signals[EDFLIB_MAXSIGNALS];
    int       edf;
    int       edfplus;
    int       bdf;
    int       bdfplus;
    int       discontinuous;
    int       signal_write_sequence_pos;
    long long starttime_offset;
    double    data_record_duration;
    long long long_data_record_duration;
    int       annots_in_file;
    int       annotlist_sz;
    int       total_annot_bytes;
    int       eq_sf;
    char      *wrbuf;
    int       wrbufsize;
    struct edfparamblock *edfparam;
};

/*
int parse_edf(const char *path)
{
    struct edfhdrblock *hdr;
    char *edf_hdr,
    scratchpad[128],
    scratchpad2[64];

    edf_hdr = (char *)calloc(1, 256);
    //edf_hdr_struct *edfhdr;
    edfhdr = (struct edfhdrblock *)calloc(1, sizeof(struct edfhdrblock));

}
*/