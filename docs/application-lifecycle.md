# Shared field application lifecycle

The port uses the original TaskManager and OverlayManager as its common
application lifecycle. Native applications are driven by `hg_field_tasks_tick`:
run the original task frame, advance the original application manager through
init/main/exit, delete a completed manager, then allow the waiting task to resume
on the next frame. Each active application retains its own typed arguments;
original game code owns acquisition and field return.

The starter now uses this path end to end. The unchanged original
ChooseStarter_LaunchApp launcher supplies its original callback descriptor.
Its presentation callbacks are statically bound to the existing native picker;
original DS overlay relocation and NNS graphics do not run. The unchanged
CreateStarter task polls the real FieldSystem application slot. Its former
pending-boolean replacement for ApplicationIsRunning has been removed.
Selection closes the native picker, the original manager completes its exit,
and only then does the original task acquire the Pokemon and finish returning
to the field. Argument cleanup remains in the original task.

Starter fades also use the same original brightness-controller closure and
scene-owned FadeWork as script FadeScreen/WaitFade. The separate starter fade
countdown, floating-point brightness calculation and renderer multiplication
were removed. A modal application draws above the faded field; ordinary field
script fades still affect the complete field surface. Standalone component
execution supplies a private fade owner without changing the controller.

## Verification and remaining work

The starter scenarios check all three choices, waiting without premature party
acquisition, actual application occupancy, shared scene-fade ownership, neutral
brightness on return, and complete task/application cleanup. Cancellation drains
the original task privately, retains RNG and cannot publish a Pokemon into the
source owner. The original bedroom-to-Mr. Pokemon scenario still passes for all
three starters. The native-window smoke and normal/ASan/UBSan suites pass.

Elm's scripted incoming call now uses the same application slot and original
OverlayManager lifecycle, with the original call controller, actual messages,
user completion and original script return. See [Elm's call](elm-phone.md).
The rival TrainerBattle command is now the next blocker. General Pokegear,
battle, save and menu applications remain unported.
Field Yes/No prompts remain original field-script interactions; they do not
need to be turned into unrelated applications to share ownership and input
control. The common runtime should preserve those original distinctions.
