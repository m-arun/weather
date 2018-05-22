/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.0-dev at Thu Apr 12 15:00:21 2018. */

#ifndef PB_WEATHER_PB_H_INCLUDED
#define PB_WEATHER_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _weatherProto {
    float MPH;
    float KmPH;
    float calDirection;
    float rainFall;
/* @@protoc_insertion_point(struct:weatherProto) */
} weatherProto;

/* Default values for struct fields */

/* Initializer values for message structs */
#define weatherProto_init_default                {0, 0, 0, 0}
#define weatherProto_init_zero                   {0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define weatherProto_MPH_tag                     1
#define weatherProto_KmPH_tag                    2
#define weatherProto_calDirection_tag            3
#define weatherProto_rainFall_tag                4

/* Struct field encoding specification for nanopb */
extern const pb_field_t weatherProto_fields[5];

/* Maximum encoded size of messages (where known) */
#define weatherProto_size                        20

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define WEATHER_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
