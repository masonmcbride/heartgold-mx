#pragma once
#include "ds_transform.h"
#include "ds_texture_animation.h"
#include <stddef.h>
#include <stdint.h>
/* Borrowed, bounded NSBCA view. Supports constant/every-frame Q12 translation and scale, and pivot
 * rotations. Reduced-rate curves/model fallback fail; scaled multi-node models
 * require unported hierarchy compensation and are rejected by the renderer. */
typedef struct { const uint8_t *bytes;size_t size;unsigned frames,nodes; } DSJointAnimation;
typedef struct { DSTransform nodes[32];uint32_t mask,scale_mask; } DSJointPose;
int ds_joint_animation_open(const uint8_t *,size_t,DSJointAnimation *);
int ds_joint_animation_pose(const DSJointAnimation *,unsigned frame,DSJointPose *);

typedef struct { unsigned model;double world[3];DSJointPose pose;DSTextureAnimation texture;unsigned texture_frame; } HGPropPose;
