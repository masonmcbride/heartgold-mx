#pragma once
#include <stddef.h>
#include "trainer_event_opcodes.inc"
typedef struct { unsigned bank,message;int found; } HGTrainerMessageRef;
/* Original trainer/message-pair lookup; missing pairs yield found=0. Invalid
 * inputs/resources preserve output. Presentation retains its existing owner. */
int hg_trainer_message_ref(unsigned trainer,unsigned message,HGTrainerMessageRef *);
