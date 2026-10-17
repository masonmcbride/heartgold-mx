cmake_minimum_required(VERSION 3.24)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Native target")
project(HeartGoldMX LANGUAGES C OBJC)
if(NOT APPLE OR NOT CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL_ERROR "Use Apple Clang on macOS")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(HG_UPSTREAM "${CMAKE_SOURCE_DIR}/upstream/pokeheartgold" CACHE PATH "Unmodified decompilation")
if(NOT EXISTS "${HG_UPSTREAM}/src/item.c")
  message(FATAL_ERROR "Missing upstream checkout; see README")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(GEN "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN}")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/item.c" "${HG_UPSTREAM}/files/itemtool/itemdata/item_data.csv" "${HG_UPSTREAM}/files/itemtool/itemdata/item_data.txt" "${CMAKE_SOURCE_DIR}/scripts/prepare.py")
# Locally authored SDK scalar interface; no proprietary SDK headers/libraries.
add_library(hg_sdk_types INTERFACE)
target_include_directories(hg_sdk_types INTERFACE "${CMAKE_SOURCE_DIR}/src/compat/include")
add_library(hg_logic STATIC "${HG_UPSTREAM}/src/item.c" "${HG_UPSTREAM}/src/bag.c" src/compat/runtime.c src/compat/menu.c src/compat/resources.c src/demo_bridge.c "${HG_UPSTREAM}/src/list_menu.c")
target_include_directories(hg_logic PUBLIC "${HG_UPSTREAM}/include" "${GEN}" "${CMAKE_SOURCE_DIR}/src")
target_compile_options(hg_logic PUBLIC "$<$<COMPILE_LANGUAGE:C>:-include>" "$<$<COMPILE_LANGUAGE:C>:${CMAKE_SOURCE_DIR}/src/compat/native.h>")
add_executable(heartgold_native src/main.c)
target_link_libraries(heartgold_native PRIVATE hg_logic)
enable_testing()
add_executable(test_items tests/test_items.c)
target_link_libraries(test_items PRIVATE hg_logic)
add_test(NAME original_items COMMAND test_items)
add_library(ds_platform STATIC src/ds_graphics.c src/ds_resources.c src/ds_model.c src/ds_texture.c src/ds_raster.c src/ds_transform.c src/ds_frame_clock.c)
target_include_directories(ds_platform PUBLIC src)
# These per-pixel loops run in the interactive app even in a Debug build.
# Keep assertions/debug symbols/sanitizers; do not enable fast-math.
option(HG_OPTIMIZE_RASTER "Optimize native CPU raster and texture sampling" ON)
if(HG_OPTIMIZE_RASTER)
  set_property(SOURCE src/ds_raster.c src/ds_texture.c APPEND PROPERTY COMPILE_OPTIONS -O2)
endif()
add_executable(test_platform tests/test_platform.c)
target_link_libraries(test_platform PRIVATE ds_platform)
add_test(NAME ds_platform COMMAND test_platform)
add_executable(heartgold_metal src/metal_main.m)
target_compile_options(heartgold_metal PRIVATE -fobjc-arc)
target_link_libraries(heartgold_metal PRIVATE hg_logic ds_platform "-framework Cocoa" "-framework Metal" "-framework MetalKit" "-framework GameController")
add_test(NAME original_icon_decode COMMAND heartgold_metal --decode-only)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/pack_resources.py" "${HG_UPSTREAM}" "${GEN}/resources" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB ICON_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/itemtool/itemdata/item_icon/*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ICON_INPUTS} "${CMAKE_SOURCE_DIR}/scripts/pack_resources.py" "${HG_UPSTREAM}/files/msgdata/msg/msg_0221.json" "${HG_UPSTREAM}/files/msgdata/msg/msg_0222.json" "${HG_UPSTREAM}/files/msgdata/msg/msg_0251.json")
target_compile_definitions(hg_logic PRIVATE HG_PACKED_ROOT="${GEN}/resources")
target_link_libraries(hg_logic PUBLIC hg_sdk_types PRIVATE ds_platform)
target_sources(hg_logic PRIVATE src/inventory.c)
add_executable(test_resources tests/test_resources.c)
target_link_libraries(test_resources PRIVATE ds_platform)
add_test(NAME resource_formats COMMAND test_resources)
add_executable(test_inventory tests/test_inventory.c)
target_link_libraries(test_inventory PRIVATE hg_logic)
add_test(NAME original_menu_inventory COMMAND test_inventory)

target_sources(hg_logic PRIVATE "${HG_UPSTREAM}/src/font_data.c" "${HG_UPSTREAM}/src/text.c" "${HG_UPSTREAM}/src/string_control_code.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/graphic/font.narc" "${HG_UPSTREAM}/files/a/0/1/5" "${HG_UPSTREAM}/tools/msgenc/charmap.txt")
target_sources(ds_platform PRIVATE src/ds_scene_graphics.c)
target_sources(hg_logic PRIVATE src/game_font.c src/game_scene.c)
add_executable(test_scene tests/test_scene.c)
target_link_libraries(test_scene PRIVATE hg_logic)
add_test(NAME original_font_sprite_scene COMMAND test_scene)

# Compile-only corpus audit: never links or executes original upstream game code.
add_custom_target(audit_native
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/audit_native.py"
    --upstream "${HG_UPSTREAM}"
    --generated "${GEN}"
    --output "${CMAKE_BINARY_DIR}/audit"
    --compiler "${CMAKE_C_COMPILER}"
    --runtime-library "$<TARGET_FILE:hg_logic>"
    --field-library "$<TARGET_FILE:hg_field>"
  DEPENDS hg_logic
  USES_TERMINAL
  VERBATIM)

# Deliberately does not use the forced native header: direct upstream SDK includes
# must work through the scalar compatibility header on their own.
add_executable(test_sdk_types tests/test_sdk_types.c)
target_include_directories(test_sdk_types PRIVATE "${HG_UPSTREAM}/include")
target_link_libraries(test_sdk_types PRIVATE hg_sdk_types)
add_test(NAME ds_scalar_abi COMMAND test_sdk_types)

target_sources(hg_logic PRIVATE src/compat/scheduler.c
  "${HG_UPSTREAM}/src/sys_task.c"
  "${HG_UPSTREAM}/src/sys_task_api.c"
  "${HG_UPSTREAM}/src/systask_environment.c")
add_executable(test_scheduler tests/test_scheduler.c)
target_link_libraries(test_scheduler PRIVATE hg_logic)
add_test(NAME original_task_scheduler COMMAND test_scheduler)

# Field resource preparation runs after general generation; its message indices
# come from the complete original message-bank manifest.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_field.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB MATRIX_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/fielddata/mapmatrix/map_matrix/*.bin")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${MATRIX_INPUTS}
 "${CMAKE_SOURCE_DIR}/scripts/prepare_field.py"
 "${HG_UPSTREAM}/files/fielddata/build_model/bm_field_matshp.dat"
 "${HG_UPSTREAM}/files/fielddata/build_model/bm_room_matshp.dat" "${HG_UPSTREAM}/files/a/1/4/8"
 "${HG_UPSTREAM}/files/a/0/4/0" "${HG_UPSTREAM}/files/a/0/4/3"
 "${HG_UPSTREAM}/files/a/0/4/2" "${HG_UPSTREAM}/files/a/0/4/4"
 "${HG_UPSTREAM}/files/a/0/6/5"
 "${HG_UPSTREAM}/files/fielddata/mapmatrix/map_matrix/meson.build"
 "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event/meson.build"
 "${HG_UPSTREAM}/files/fielddata/script/scr_seq/meson.build"
 "${HG_UPSTREAM}/files/msgdata/NARC_msg.txt"
 "${HG_UPSTREAM}/files/fielddata/encountdata/gs_enc_data.json")
add_library(hg_field STATIC "${HG_UPSTREAM}/src/map_matrix.c" "${HG_UPSTREAM}/src/map_header.c" "${HG_UPSTREAM}/src/metatile_behavior.c" src/compat/field.c src/land_data.c src/field_preview.c)
target_link_libraries(hg_field PUBLIC hg_logic)
target_include_directories(hg_field PRIVATE "${GEN}/files")
target_compile_options(hg_field PUBLIC "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/field.h\">")
add_executable(heartgold_field src/field_main.c)
target_link_libraries(heartgold_field PRIVATE hg_field)
add_executable(test_field tests/test_field.c)
target_link_libraries(test_field PRIVATE hg_field)
add_test(NAME original_field_data COMMAND test_field)
add_test(NAME field_cli_invalid COMMAND heartgold_field --map 540)
set_tests_properties(field_cli_invalid PROPERTIES WILL_FAIL TRUE)

add_dependencies(audit_native hg_field)

add_executable(test_model tests/test_model.c)
target_link_libraries(test_model PRIVATE ds_platform)
add_test(NAME model_geometry COMMAND test_model)

add_executable(heartgold_field_viewer src/field_viewer.m)
target_compile_options(heartgold_field_viewer PRIVATE -fobjc-arc)
target_link_libraries(heartgold_field_viewer PRIVATE "-framework Cocoa")
add_executable(test_raster tests/test_raster.c)
target_link_libraries(test_raster PRIVATE ds_platform)
add_test(NAME textured_raster_semantics COMMAND test_raster)
add_executable(test_field_view tests/test_field_view.c)
target_link_libraries(test_field_view PRIVATE hg_field_view)
add_test(NAME original_textured_field_view COMMAND test_field_view)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_props.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_props.py" "${HG_UPSTREAM}/include/field/overlay_01_02204004.h" "${HG_UPSTREAM}/include/unk_02097268.h" "${HG_UPSTREAM}/include/field/map_load_manager.h")
add_library(hg_props STATIC "${HG_UPSTREAM}/src/field/map_prop_manager.c" src/compat/props.c)
target_link_libraries(hg_props PUBLIC hg_field)
target_compile_options(hg_props PUBLIC "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/props.h\">")
set_source_files_properties("${HG_UPSTREAM}/src/field/map_prop_manager.c" PROPERTIES COMPILE_OPTIONS "-finstrument-functions")

add_executable(heartgold_field_lifecycle src/field_lifecycle.c)
target_link_libraries(heartgold_field_lifecycle PRIVATE hg_props)
add_test(NAME original_prop_lifecycle COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/field_evidence.py" --build "${CMAKE_BINARY_DIR}" --upstream "${HG_UPSTREAM}" --scenario "${CMAKE_SOURCE_DIR}/scenarios/new-bark-props.json")
add_custom_target(field_evidence
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/field_evidence.py" --build "${CMAKE_BINARY_DIR}" --upstream "${HG_UPSTREAM}" --scenario "${CMAKE_SOURCE_DIR}/scenarios/new-bark-props.json"
  DEPENDS heartgold_field_lifecycle VERBATIM USES_TERMINAL)
add_test(NAME field_evidence_validation COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_field_evidence.py")

add_library(hg_field_view STATIC src/field_view.c)
target_link_libraries(hg_field_view PUBLIC hg_props)

# Original Thumb instructions, assembled without SDK or ROM. GNU divided syntax
# is adapted only at the assembler boundary; generated bytes stay build-local.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_thumb.py"
  "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/scripts/prepare_thumb.py" "${HG_UPSTREAM}/asm/overlay_01_021FAD1C.s" "${HG_UPSTREAM}/asm/unk_02020B8C.s")
target_sources(ds_platform PRIVATE src/ds_thumb.c)
add_executable(test_thumb tests/test_thumb.c)
target_include_directories(test_thumb PRIVATE "${GEN}")
target_link_libraries(test_thumb PRIVATE hg_field)
add_test(NAME original_thumb_terrain COMMAND test_thumb)
target_sources(hg_field PRIVATE src/terrain_guest.c)

add_executable(heartgold_field_catalog src/field_catalog.c)
target_link_libraries(heartgold_field_catalog PRIVATE hg_field_view)
add_test(NAME field_catalog_new_bark COMMAND heartgold_field_catalog 60)
set_tests_properties(field_catalog_new_bark PROPERTIES PASS_REGULAR_EXPRESSION "\"rendered\":true")

# Original application lifecycle. Module residency is a native boundary;
# compiling this manager does not compile the original overlay entry points.
add_library(hg_overlays STATIC "${HG_UPSTREAM}/src/overlay_manager.c" src/compat/overlays.c)
target_link_libraries(hg_overlays PUBLIC hg_logic)
target_compile_options(hg_overlays PUBLIC "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/overlays.h\">")
add_executable(test_overlays tests/test_overlays.c)
add_test(NAME original_overlay_lifecycle COMMAND test_overlays)
add_library(hg_field_app STATIC src/field_app.c)
target_link_libraries(hg_field_app PUBLIC hg_field_view PRIVATE hg_overlays)
target_link_libraries(heartgold_field_viewer PRIVATE hg_field_app)

target_link_libraries(test_overlays PRIVATE hg_field_app)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_characters.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB CHARACTER_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/data/mmodel/mmodel/*.NSBMD" "${HG_UPSTREAM}/files/data/mmodel/mmodel/*.NSBTX")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CHARACTER_INPUTS} "${CMAKE_SOURCE_DIR}/scripts/prepare_characters.py" "${HG_UPSTREAM}/include/constants/mmodel.h" "${HG_UPSTREAM}/asm/overlay_01_sprite_data.s")
add_executable(test_characters tests/test_characters.c)
target_link_libraries(test_characters PRIVATE hg_field_view)
add_test(NAME character_sprite_catalog COMMAND test_characters)

add_library(hg_field_tasks STATIC "${HG_UPSTREAM}/src/task.c" src/compat/field_tasks.c)
target_link_libraries(hg_field_tasks PUBLIC hg_overlays)
target_compile_options(hg_field_tasks PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/field_tasks.h\">")
target_link_libraries(hg_field_app PRIVATE hg_field_tasks)
add_executable(test_field_tasks tests/test_field_tasks.c)
target_link_libraries(test_field_tasks PRIVATE hg_field_app hg_field_tasks)
add_test(NAME original_field_task_flow COMMAND test_field_tasks)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_motion.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_transitions.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_transitions.py"
  "${HG_UPSTREAM}/include/map_events_internal.h" "${HG_UPSTREAM}/include/field_system.h"
  "${HG_UPSTREAM}/include/field_types_def.h" "${HG_UPSTREAM}/include/map_events.h" "${HG_UPSTREAM}/include/save_local_field_data.h"
  "${HG_UPSTREAM}/src/map_events.c" "${HG_UPSTREAM}/src/save_local_field_data.c"
  "${HG_UPSTREAM}/src/field/field_control.c" "${HG_UPSTREAM}/src/field_warp_tasks.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_motion.py" "${HG_UPSTREAM}/include/map_events_internal.h")
add_library(hg_motion STATIC "${HG_UPSTREAM}/src/map_object.c" "${HG_UPSTREAM}/src/player_avatar.c")
target_sources(hg_motion PRIVATE src/field_transition.c "${HG_UPSTREAM}/src/save_local_field_data.c")
target_compile_definitions(hg_motion PRIVATE HG_ORIGINAL_MAP_EVENTS="${HG_UPSTREAM}/src/map_events.c")
set_property(SOURCE "${HG_UPSTREAM}/src/save_local_field_data.c" APPEND PROPERTY COMPILE_OPTIONS
  -include "${CMAKE_SOURCE_DIR}/src/compat/transitions.h")
target_link_libraries(hg_motion PUBLIC hg_field_tasks hg_props)
target_compile_options(hg_motion PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/motion.h\">")
add_executable(test_motion tests/test_motion.c)
target_compile_options(test_motion PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/motion.h\">")
target_link_libraries(test_motion PRIVATE hg_motion)
target_link_options(hg_motion INTERFACE -Wl,-dead_strip)
add_library(hg_stats STATIC "${HG_UPSTREAM}/src/game_stats.c" "${HG_UPSTREAM}/src/math_util.c" src/compat/stats.c)
target_link_libraries(hg_stats PUBLIC hg_props)
target_compile_options(hg_stats PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/stats.h\">")
target_link_options(hg_stats INTERFACE -Wl,-dead_strip)
target_link_libraries(hg_motion PUBLIC hg_stats)
add_executable(test_stats tests/test_stats.c)
target_link_libraries(test_stats PRIVATE hg_stats)
add_test(NAME original_game_statistics COMMAND test_stats)
add_executable(test_locomotion tests/test_locomotion.c)
target_link_libraries(test_locomotion PRIVATE hg_motion)
add_test(NAME original_locomotion_controller COMMAND test_locomotion)
add_executable(test_transitions tests/test_transitions.c)
target_link_libraries(test_transitions PRIVATE hg_field_app hg_motion)
add_test(NAME original_field_transitions COMMAND test_transitions)
add_test(NAME original_motion_state COMMAND test_motion)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_field_guest.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_field_guest.py"
  "${HG_UPSTREAM}/asm/unk_0205CB48.s" "${HG_UPSTREAM}/asm/unk_0205FD20.s"
  "${HG_UPSTREAM}/asm/unk_02054648.s" "${HG_UPSTREAM}/asm/unk_02062108.s"
  "${HG_UPSTREAM}/asm/unk_data_020FCBD8.s" "${HG_UPSTREAM}/asm/unk_data_020FDB44.s"
  "${HG_UPSTREAM}/asm/unk_data_020FD978.s"
  "${HG_UPSTREAM}/asm/overlay_01_022053EC.s"
  "${HG_UPSTREAM}/asm/unk_0203BA5C.s"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0096_D31R0201.json"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0066_D23R0102.json")
target_sources(hg_motion PRIVATE src/motion_guest.c)

target_sources(hg_motion PRIVATE src/field_player.c)
target_link_libraries(hg_field_app PRIVATE hg_motion)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_interaction.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB HG_EVENT_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event/*.json" "${HG_UPSTREAM}/files/fielddata/script/scr_seq/event_*.h")
file(GLOB_RECURSE HG_JSONPROC_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/tools/jsonproc/*.h" "${HG_UPSTREAM}/tools/jsonproc/*.hpp" "${HG_UPSTREAM}/tools/jsonproc/*.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HG_EVENT_INPUTS} ${HG_JSONPROC_INPUTS}
  "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event.json.txt"
  "${HG_UPSTREAM}/asm/macros/script.inc" "${HG_UPSTREAM}/asm/macros/movement.inc"
  "${CMAKE_SOURCE_DIR}/scripts/prepare_interaction.py" "${CMAKE_SOURCE_DIR}/scripts/narc_tools.py"
  "${HG_UPSTREAM}/include/script.h" "${HG_UPSTREAM}/asm/unk_0203DB6C.s"
  "${HG_UPSTREAM}/files/fielddata/script/scr_seq/scr_seq_0842_T20.s"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0542_T20.json")
add_library(hg_script STATIC "${HG_UPSTREAM}/src/script.c" src/sign_script.c)
target_link_libraries(hg_script PUBLIC hg_field)
target_compile_options(hg_script PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/script.h\">")
target_sources(hg_field PRIVATE src/field_events.c)
target_link_libraries(hg_motion PUBLIC hg_script)

add_executable(test_interaction tests/test_interaction.c)
target_compile_options(test_interaction PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/motion.h\">")
target_link_libraries(test_interaction PRIVATE hg_field_app hg_motion hg_script)
add_test(NAME original_background_interaction COMMAND test_interaction)

add_executable(heartgold_field_benchmark src/field_benchmark.c)
target_link_libraries(heartgold_field_benchmark PRIVATE hg_field_app)

# Read-only symbol/provider evidence. A complete build supplies all objects and
# executables; test linkage is explicitly not reported as execution coverage.
add_custom_target(reuse_audit
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/audit_reuse.py"
    --build "${CMAKE_BINARY_DIR}" --upstream "${HG_UPSTREAM}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" VERBATIM USES_TERMINAL)
get_property(HG_AUDIT_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(HG_AUDIT_TARGET IN LISTS HG_AUDIT_TARGETS)
  get_target_property(HG_AUDIT_TYPE "${HG_AUDIT_TARGET}" TYPE)
  if(HG_AUDIT_TYPE STREQUAL "EXECUTABLE")
    add_dependencies(reuse_audit "${HG_AUDIT_TARGET}")
  endif()
endforeach()
