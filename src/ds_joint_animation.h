#pragma once
#include "ds_transform.h"
#include <stddef.h>
#include <stdint.h>
/* Borrowed, bounded NSBCA view. Current slice: unit translation/scale and
 * constant or every-frame pivot rotations. Other curves fail admission. */
typedef struct { const uint8_t *bytes;size_t size;unsigned frames,nodes; } DSJointAnimation;
typedef struct { DSTransform nodes[32];uint32_t mask; } DSJointPose;
int ds_joint_animation_open(const uint8_t *,size_t,DSJointAnimation *);
int ds_joint_animation_pose(const DSJointAnimation *,unsigned frame,DSJointPose *);

typedef struct { unsigned model;double world[3];DSJointPose pose; } HGPropPose;
