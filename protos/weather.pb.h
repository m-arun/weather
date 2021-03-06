/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.0-dev at Thu May 10 14:51:09 2018. */

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
    float windSpeed;
    float degree;
    float precipitation;
/* @@protoc_insertion_point(struct:weatherProto) */
} weatherProto;

/* Default values for struct fields */

/* Initializer values for message structs */
#define weatherProto_init_default                {0, 0, 0}
#define weatherProto_init_zero                   {0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define weatherProto_windSpeed_tag               1
#define weatherProto_degree_tag                  2
#define weatherProto_precipitation_tag           3

/* Struct field encoding specification for nanopb */
extern const pb_field_t weatherProto_fields[4];

/* Maximum encoded size of messages (where known) */
#define weatherProto_size                        15

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define WEATHER_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
