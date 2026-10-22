#pragma once
/* Registry callbacks execute the original assembly on the shared Thumb runtime.
 * Unresolved service calls terminate with the original service name. */
const char *hg_save_records_last_error(void);
