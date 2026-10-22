#pragma once
#include "compat/save_c_records.h"
#include "compat/field_tasks.h"
#include "bug_contest_internal.h"
/* Volatile original contest owner. The original menu forbids saving while it
 * exists: the backup party and retained catch are not registered save records. */
BugContest *hg_bug_contest_new(SaveData *,unsigned weekday,char *error,size_t);
/* Preview and rollback free only volatile owners, never restore a party. */
BugContest *hg_bug_contest_clone(const BugContest *);
void hg_bug_contest_discard(BugContest *);
void hg_bug_contest_rebind(BugContest *,SaveData *);
void hg_bug_contest_delete(BugContest *);
void hg_bug_contest_judge(BugContest *);
/* Copy the original slot, free its short-lived allocation. */
int hg_bug_contest_encounter(BugContest *,unsigned *species,unsigned *level);
/* User selection from the original keep/replace application. No catch goes
 * directly into party or PC before original contest deletion. */
int hg_bug_contest_keep(BugContest *,const Pokemon *caught,int replace);
int hg_bug_contest_registered(BugContest *,unsigned id);
