#ifndef _INTEL_H
#define _INTEL_H
#pragma pack(1)
/* sidt instruction stores the base and limit of IDTR in this format */
typedef struct idtr {
        short Limit;
        unsigned int Base;
} Idtr_t, *PIdtr_t;

/* Decriptor Entry corresponding to interrupt gate */
typedef struct idtentry {
        unsigned short OffsetLow;
        unsigned short Selector;
        unsigned char Reserved;
        unsigned char Type:4;
        unsigned char Always0:1;
        unsigned char Dpl:2;
        unsigned char Present:1;
        unsigned short OffsetHigh;
} IdtEntry_t, *PIdtEntry_t;

typedef struct
{
    unsigned short  limit_0_15;
    unsigned short  base_0_15;
    unsigned char   base_16_23;

    unsigned char    accessed    : 1;
    unsigned char    readable    : 1;
    unsigned char    conforming  : 1;
    unsigned char    code_data   : 1;
    unsigned char    app_system  : 1;
    unsigned char    dpl         : 2;
    unsigned char    present     : 1;

    unsigned char    limit_16_19 : 4;
    unsigned char    unused      : 1;
    unsigned char    always_0    : 1;
    unsigned char    seg_16_32   : 1;
    unsigned char    granularity : 1;

    unsigned char   base_24_31;
} CODE_SEG_DESCRIPTOR;

typedef struct
{
    unsigned short  offset_0_15;
    unsigned short  selector;

    unsigned char    param_count : 4;
    unsigned char    some_bits   : 4;

    unsigned char    type        : 4;
    unsigned char    app_system  : 1;
    unsigned char    dpl         : 2;
    unsigned char    present     : 1;

    unsigned short  offset_16_31;
} CALLGATE_DESCRIPTOR;

#pragma pack()
#endif
