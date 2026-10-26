cmake_minimum_required(VERSION 3.24)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Native target")
project(HeartGoldMX LANGUAGES C CXX OBJC)
if(NOT APPLE OR NOT CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL_ERROR "Use Apple Clang on macOS")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/generated_files.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/native_bit_operations.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_original_parent.py")
set(HG_UPSTREAM "${CMAKE_SOURCE_DIR}/upstream/pokeheartgold" CACHE PATH "Unmodified decompilation")
if(NOT EXISTS "${HG_UPSTREAM}/src/item.c")
  message(FATAL_ERROR "Missing upstream checkout; see README")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(GEN "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN}")
file(GLOB HG_MESSAGE_INDEX_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/msgdata/msg/*.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HG_MESSAGE_INDEX_INPUTS})
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/item.c" "${HG_UPSTREAM}/files/itemtool/itemdata/item_data.csv" "${HG_UPSTREAM}/files/itemtool/itemdata/item_data.txt" "${CMAKE_SOURCE_DIR}/scripts/prepare.py")
# Native scalar interface. Original guest 2D code below uses pinned build-local SDK inputs.
add_library(hg_sdk_types INTERFACE)
target_include_directories(hg_sdk_types INTERFACE "${CMAKE_SOURCE_DIR}/src/compat/include")
add_library(hg_logic STATIC "${HG_UPSTREAM}/src/item.c" "${HG_UPSTREAM}/src/bag.c" src/compat/runtime.c src/compat/native_heap.c src/compat/menu.c src/compat/resources.c src/demo_bridge.c "${HG_UPSTREAM}/src/list_menu.c")
target_include_directories(hg_logic PUBLIC "${HG_UPSTREAM}/include" "${GEN}" "${CMAKE_SOURCE_DIR}/src")
target_compile_options(hg_logic PUBLIC "$<$<COMPILE_LANGUAGE:C>:-include>" "$<$<COMPILE_LANGUAGE:C>:${CMAKE_SOURCE_DIR}/src/compat/native.h>")
add_executable(heartgold_native src/main.c)
target_link_libraries(heartgold_native PRIVATE hg_logic)
enable_testing()
add_executable(test_items tests/test_items.c)
target_link_libraries(test_items PRIVATE hg_logic)
add_test(NAME original_items COMMAND test_items)
add_library(ds_platform STATIC src/ds_video.c src/ds_irq.c src/ds_brightness.c src/ds_graphics.c src/ds_resources.c src/ds_model.c src/ds_joint_animation.c src/ds_texture_animation.c src/ds_texture.c src/ds_raster.c src/ds_transform.c src/ds_frame_clock.c)
target_include_directories(ds_platform PUBLIC src)
# These per-pixel loops run in the interactive app even in a Debug build.
# Keep assertions/debug symbols/sanitizers; do not enable fast-math.
option(HG_OPTIMIZE_RASTER "Optimize native CPU raster and texture sampling" ON)
if(HG_OPTIMIZE_RASTER)
  set_property(SOURCE src/ds_raster.c src/ds_texture.c APPEND PROPERTY COMPILE_OPTIONS -O2)
endif()
add_executable(test_ds_mapping tests/test_ds_mapping.c)
target_include_directories(test_ds_mapping PRIVATE src)
add_test(NAME ds_memory_correspondence COMMAND test_ds_mapping)
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
target_sources(hg_logic PRIVATE src/game_font.c src/game_scene.c src/game_name.c)
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
    --runtime-library "$<TARGET_FILE:hg_motion>"
    --runtime-library "$<TARGET_FILE:hg_script>"
    --runtime-library "$<TARGET_FILE:hg_stats>"
    --runtime-library "$<TARGET_FILE:hg_pokemon>"
    --runtime-library "$<TARGET_FILE:hg_props>"
    --runtime-library "$<TARGET_FILE:hg_field_tasks>"
    --runtime-library "$<TARGET_FILE:hg_overlays>"
    --runtime-library "$<TARGET_FILE:hg_field_app>"
    --runtime-library "$<TARGET_FILE:hg_field_view>"
    --runtime-library "$<TARGET_FILE:ds_platform>"
    --field-library "$<TARGET_FILE:hg_field>"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/audit_workstreams.py"
    --upstream "${HG_UPSTREAM}" --audit "${CMAKE_BINARY_DIR}/audit"
  DEPENDS hg_logic
  USES_TERMINAL
  VERBATIM)

# Deliberately does not use the forced native header: direct upstream SDK includes
# must work through the scalar compatibility header on their own.
add_executable(test_sdk_types tests/test_sdk_types.c)
target_include_directories(test_sdk_types PRIVATE "${HG_UPSTREAM}/include")
target_link_libraries(test_sdk_types PRIVATE hg_sdk_types)
add_test(NAME ds_scalar_abi COMMAND test_sdk_types)

target_sources(hg_logic PRIVATE src/compat/scheduler.c src/compat/native_irq.c src/compat/native_video.c src/compat/native_vram.c
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
 "${HG_UPSTREAM}/files/a/0/6/5" "${HG_UPSTREAM}/files/a/1/0/6" "${HG_UPSTREAM}/files/a/1/0/7" "${HG_UPSTREAM}/files/a/1/0/8"
 "${HG_UPSTREAM}/files/fielddata/mapmatrix/map_matrix/meson.build"
 "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event/meson.build"
 "${HG_UPSTREAM}/files/fielddata/script/scr_seq/meson.build"
 "${HG_UPSTREAM}/files/data/area00light.txt" "${HG_UPSTREAM}/files/data/area01light.txt"
 "${HG_UPSTREAM}/files/data/area02light.txt" "${HG_UPSTREAM}/files/data/dun20_01light.txt" "${HG_UPSTREAM}/files/data/dun20_02light.txt"
 "${HG_UPSTREAM}/files/msgdata/NARC_msg.txt"
 "${HG_UPSTREAM}/files/fielddata/encountdata/gs_enc_data.json")
add_library(hg_field STATIC "${HG_UPSTREAM}/src/map_matrix.c" "${HG_UPSTREAM}/src/map_header.c" "${HG_UPSTREAM}/src/metatile_behavior.c" src/compat/field.c src/land_data.c src/field_preview.c)
target_sources(hg_field PRIVATE src/compat/story_records.c)
target_compile_definitions(hg_field PRIVATE HG_ORIGINAL_SAVE_FLAGS="${HG_UPSTREAM}/src/save_vars_flags.c")
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

add_executable(heartgold_field_viewer src/field_viewer.m tests/field_ui_routes.c)
target_compile_options(heartgold_field_viewer PRIVATE "$<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>")
target_link_libraries(heartgold_field_viewer PRIVATE "-framework Cocoa")
add_executable(test_raster tests/test_raster.c)
target_link_libraries(test_raster PRIVATE ds_platform)
add_test(NAME textured_raster_semantics COMMAND test_raster)
add_executable(test_field_view tests/test_field_view.c)
target_link_libraries(test_field_view PRIVATE hg_field_view)
add_test(NAME original_textured_field_view COMMAND test_field_view)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_props.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_props.py" "${HG_UPSTREAM}/include/gf_rtc.h" "${HG_UPSTREAM}/include/field/overlay_01_02204004.h" "${HG_UPSTREAM}/include/unk_02097268.h" "${HG_UPSTREAM}/include/field/map_load_manager.h" "${HG_UPSTREAM}/include/field/map_prop_animation.h" "${HG_UPSTREAM}/src/field/map_prop_animation.c" "${HG_UPSTREAM}/src/field/overlay_01_02204004.c")
add_library(hg_props STATIC "${HG_UPSTREAM}/src/field/map_prop_manager.c" src/compat/props.c src/compat/prop_animation.c)
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
  "${CMAKE_SOURCE_DIR}/scripts/prepare_thumb.py" "${HG_UPSTREAM}/asm/overlay_01_021FAD1C.s" "${HG_UPSTREAM}/asm/unk_02020B8C.s" "${HG_UPSTREAM}/asm/unk_02023694.s" "${HG_UPSTREAM}/asm/unk_0200B150.s" "${HG_UPSTREAM}/asm/overlay_02_02248728.s"
  "${HG_UPSTREAM}/asm/overlay_01_021F1348.s" "${HG_UPSTREAM}/asm/unk_020689C8.s" "${HG_UPSTREAM}/asm/unk_0202068C.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FC4C4.s" "${HG_UPSTREAM}/asm/overlay_01_data_02208BFC.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FD1B8.s" "${HG_UPSTREAM}/asm/overlay_01_02203E40.s"
  "${HG_UPSTREAM}/src/unk_02025534.c" "${HG_UPSTREAM}/include/gf_3d_loader.h")
target_sources(ds_platform PRIVATE src/ds_thumb.c)
# Profiled opening-route execution spends ~20s dispatching 501M guest instructions.
# Optimize that shared machine service, retaining assertions and sanitizer flags.
option(HG_OPTIMIZE_THUMB "Optimize bounded ARM/Thumb interpreter dispatch" ON)
if(HG_OPTIMIZE_THUMB)
  set_property(SOURCE src/ds_thumb.c APPEND PROPERTY COMPILE_OPTIONS -O2)
endif()

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
add_executable(test_native_lifetimes tests/test_native_lifetimes.c)
target_link_libraries(test_native_lifetimes PRIVATE hg_overlays)
add_test(NAME native_heap_module_lifetimes COMMAND test_native_lifetimes)
# Isolated original native owner; never links the legacy field-task owner.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_original_field.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
include("${GEN}/original_field_inputs.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/scripts/prepare_original_field.py"
  ${HG_ORIGINAL_FIELD_INPUTS})
add_executable(test_original_field_owner tests/test_original_field_owner.c "${HG_UPSTREAM}/src/task.c")
target_link_libraries(test_original_field_owner PRIVATE hg_overlays)
target_include_directories(test_original_field_owner PRIVATE "${GEN}")
target_compile_definitions(test_original_field_owner PRIVATE POKEHEARTGOLD_SCRIPT_MANAGER_H)
target_compile_options(test_original_field_owner PRIVATE "SHELL:-include \"${GEN}/original_field_owner.h\"")
add_test(NAME original_field_owner_mechanisms COMMAND test_original_field_owner)
# These are upstream's existing pinned dependencies, read only for original
# guest software code/layouts; never linked into the native scalar interface.
set(HG_NITROSDK "${CMAKE_SOURCE_DIR}/build/evidence/weather/NitroSDK" CACHE PATH "Pinned upstream NitroSDK source, build-local")
set(HG_NITROSYSTEM "${CMAKE_SOURCE_DIR}/build/evidence/weather/NitroSystem" CACHE PATH "Pinned upstream NitroSystem source, build-local")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/scripts/prepare_original_2d.py" "${CMAKE_SOURCE_DIR}/scripts/prepare_original_2d_sdk.py"
  "${CMAKE_SOURCE_DIR}/scripts/prepare_original_sound.py"
  "${HG_UPSTREAM}/files/a/0/6/3" "${HG_UPSTREAM}/files/data/clact_default.NANR" "${HG_UPSTREAM}/files/a/1/3/9")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_field_loader.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" "${HG_NITROSDK}" "${HG_NITROSYSTEM}" COMMAND_ERROR_IS_FATAL ANY)
include("${GEN}/original_loader_dependencies.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/scripts/prepare_field_loader.py" "${CMAKE_SOURCE_DIR}/scripts/guest_assembly.py"
  "${HG_UPSTREAM}/asm/overlay_01_021F4704.s" "${HG_UPSTREAM}/asm/overlay_01_021FAC44.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FB04C.s" "${HG_UPSTREAM}/asm/overlay_01_021FB878.s"
  "${HG_UPSTREAM}/asm/overlay_01_021EA6C4.s"
  "${HG_UPSTREAM}/asm/unk_02054648.s" "${HG_UPSTREAM}/asm/overlay_01_021FAD1C.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FB368.s" "${HG_UPSTREAM}/asm/unk_02020B8C.s" "${HG_UPSTREAM}/asm/unk_02023694.s" "${HG_UPSTREAM}/asm/unk_0200B150.s" "${HG_UPSTREAM}/asm/overlay_02_02248728.s"
  "${HG_UPSTREAM}/asm/overlay_01_021F1348.s" "${HG_UPSTREAM}/asm/unk_020689C8.s" "${HG_UPSTREAM}/asm/unk_0202068C.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FC4C4.s" "${HG_UPSTREAM}/asm/overlay_01_data_02208BFC.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FD1B8.s" "${HG_UPSTREAM}/asm/overlay_01_02203E40.s"
  "${HG_UPSTREAM}/src/unk_02025534.c" "${HG_UPSTREAM}/include/gf_3d_loader.h"
  "${HG_UPSTREAM}/src/system.c" "${HG_UPSTREAM}/include/system.h"
  "${HG_UPSTREAM}/src/field/map_prop_manager.c" "${HG_UPSTREAM}/files/a/0/7/0")
add_executable(test_original_field_constructor tests/test_original_field_constructor.c src/compat/original_loader.c "${HG_UPSTREAM}/src/task.c" "${HG_UPSTREAM}/src/field/field_3d_object_task.c")
target_compile_options(test_original_field_constructor PRIVATE "SHELL:-include \"${GEN}/original_field_task_names.h\"" "SHELL:-include \"${GEN}/original_field_owner.h\"")
target_link_libraries(test_original_field_constructor PRIVATE hg_overlays)
target_compile_definitions(test_original_field_constructor PRIVATE HG_LOADER_TEST_HOOKS)
target_compile_definitions(test_original_field_constructor PRIVATE POKEHEARTGOLD_SCRIPT_MANAGER_H MI_CpuClear32=MI_CpuClear8)
target_include_directories(test_original_field_constructor PRIVATE "${GEN}")
target_compile_definitions(test_original_field_constructor PRIVATE HG_ORIGINAL_SCRIPT_VM="${HG_UPSTREAM}/src/script.c")
add_test(NAME original_field_construction COMMAND test_original_field_constructor)
add_test(NAME original_sound_owners COMMAND test_original_field_constructor --sound-owners)
add_test(NAME original_sound_commands COMMAND test_original_field_constructor --sound-commands)
add_test(NAME original_loader_files COMMAND test_original_field_constructor --files)
add_library(hg_field_app STATIC src/field_app.c src/field_menu.c src/story_checkpoints.c)
target_link_libraries(hg_field_app PUBLIC hg_field_view PRIVATE hg_overlays)
target_link_libraries(heartgold_field_viewer PRIVATE hg_field_app)

target_link_libraries(test_overlays PRIVATE hg_field_app)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_characters.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB CHARACTER_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/data/mmodel/mmodel/*.NSBMD" "${HG_UPSTREAM}/files/data/mmodel/mmodel/*.NSBTX" "${HG_UPSTREAM}/files/data/mmodel/mmodel/*.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CHARACTER_INPUTS} "${HG_UPSTREAM}/files/data/mmodel/mmodel.json.txt" "${CMAKE_SOURCE_DIR}/scripts/prepare_characters.py" "${HG_UPSTREAM}/include/constants/mmodel.h" "${HG_UPSTREAM}/include/constants/sprites.h" "${HG_UPSTREAM}/asm/overlay_01_sprite_data.s")
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
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_motion.py" "${HG_UPSTREAM}/include/map_events_internal.h" "${HG_UPSTREAM}/include/map_object.h")
add_library(hg_motion STATIC src/compat/map_objects.c src/compat/healing.c "${HG_UPSTREAM}/src/player_avatar.c")
target_compile_definitions(hg_motion PRIVATE HG_ORIGINAL_MAP_OBJECT="${HG_UPSTREAM}/src/map_object.c")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/map_object.c")
target_sources(hg_motion PRIVATE src/field_transition.c "${HG_UPSTREAM}/src/save_local_field_data.c")
target_compile_definitions(hg_motion PRIVATE HG_ORIGINAL_MAP_EVENTS="${HG_UPSTREAM}/src/map_events.c")
set_property(SOURCE "${HG_UPSTREAM}/src/save_local_field_data.c" APPEND PROPERTY COMPILE_OPTIONS
  -include "${CMAKE_SOURCE_DIR}/src/compat/transitions.h")
target_link_libraries(hg_motion PUBLIC hg_field_tasks hg_props hg_field_view)
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
set_tests_properties(original_field_transitions PROPERTIES DEPENDS original_violet_arrival RUN_SERIAL TRUE WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_executable(test_actors tests/test_actors.c)
target_link_libraries(test_actors PRIVATE hg_field_app hg_motion)
add_test(NAME original_event_actors COMMAND test_actors)
add_test(NAME original_motion_state COMMAND test_motion)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_field_guest.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_field_guest.py" "${CMAKE_SOURCE_DIR}/scripts/guest_assembly.py" "${HG_UPSTREAM}/src/metatile_behavior.c" "${HG_UPSTREAM}/asm/overlay_02_02248728.s"
  "${HG_UPSTREAM}/asm/overlay_01_021F1348.s" "${HG_UPSTREAM}/asm/unk_020689C8.s" "${HG_UPSTREAM}/asm/unk_0202068C.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FC4C4.s" "${HG_UPSTREAM}/asm/overlay_01_data_02208BFC.s"
  "${HG_UPSTREAM}/asm/overlay_01_021FD1B8.s" "${HG_UPSTREAM}/asm/overlay_01_02203E40.s"
  "${HG_UPSTREAM}/src/unk_02025534.c" "${HG_UPSTREAM}/include/gf_3d_loader.h"
  "${HG_UPSTREAM}/asm/unk_0205CB48.s" "${HG_UPSTREAM}/asm/unk_0205FD20.s"
  "${HG_UPSTREAM}/asm/unk_02054648.s" "${HG_UPSTREAM}/asm/unk_02062108.s"
  "${HG_UPSTREAM}/asm/unk_02061284.s" "${HG_UPSTREAM}/asm/unk_020632B0.s"
  "${HG_UPSTREAM}/asm/unk_data_020FCBD8.s" "${HG_UPSTREAM}/asm/unk_data_020FDB44.s"
  "${HG_UPSTREAM}/asm/unk_data_020FD978.s"
  "${HG_UPSTREAM}/asm/overlay_01_022053EC.s" "${HG_UPSTREAM}/asm/overlay_01_022001E4.s"
  "${HG_UPSTREAM}/asm/unk_0203BA5C.s"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0096_D31R0201.json"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0066_D23R0102.json")
target_sources(hg_motion PRIVATE src/motion_guest.c)

target_sources(hg_motion PRIVATE src/field_player.c src/compat/script_scene.c src/compat/field_control.c)
target_link_libraries(hg_field_app PRIVATE hg_motion)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_interaction.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
file(GLOB HG_EVENT_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event/*.json" "${HG_UPSTREAM}/files/fielddata/script/scr_seq/event_*.h")
file(GLOB HG_LOCAL_SCRIPT_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/files/fielddata/script/scr_seq/scr_seq_*.s" "${HG_UPSTREAM}/files/msgdata/msg/msg_*.json")
file(GLOB_RECURSE HG_SCRIPT_CONSTANT_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/include/constants/*.h")
file(GLOB_RECURSE HG_JSONPROC_INPUTS CONFIGURE_DEPENDS "${HG_UPSTREAM}/tools/jsonproc/*.h" "${HG_UPSTREAM}/tools/jsonproc/*.hpp" "${HG_UPSTREAM}/tools/jsonproc/*.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HG_EVENT_INPUTS} ${HG_JSONPROC_INPUTS} ${HG_LOCAL_SCRIPT_INPUTS} ${HG_SCRIPT_CONSTANT_INPUTS}
  "${HG_UPSTREAM}/files/fielddata/script/scr_seq/meson.build"
  "${HG_UPSTREAM}/src/location_backup.c" "${HG_UPSTREAM}/asm/overlay_01_021F1AFC.s"
  "${HG_UPSTREAM}/files/fielddata/eventdata/zone_event.json.txt"
  "${HG_UPSTREAM}/asm/macros/script.inc" "${HG_UPSTREAM}/asm/macros/movement.inc"
  "${CMAKE_SOURCE_DIR}/scripts/prepare_interaction.py" "${CMAKE_SOURCE_DIR}/scripts/narc_tools.py"
  "${HG_UPSTREAM}/src/script_manager.c" "${HG_UPSTREAM}/include/constants/std_script.h"
  "${HG_UPSTREAM}/include/script.h" "${HG_UPSTREAM}/asm/unk_0203DB6C.s"
  "${HG_UPSTREAM}/files/fielddata/script/scr_seq/scr_seq_0842_T20.s"
  "${HG_UPSTREAM}/files/msgdata/msg/msg_0542_T20.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_script_kernel.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_script_kernel.py"
  "${HG_UPSTREAM}/src/field_bgm.c" "${HG_UPSTREAM}/src/overlay_26_022599D0.c"
  "${HG_UPSTREAM}/src/script_manager.c" "${HG_UPSTREAM}/src/scrcmd_c.c" "${HG_UPSTREAM}/src/sys_vars.c"
  "${HG_UPSTREAM}/src/scrcmd_lottery.c" "${HG_UPSTREAM}/src/player_data.c" "${HG_UPSTREAM}/src/scrcmd_17.c"
  "${HG_UPSTREAM}/src/party.c" "${HG_UPSTREAM}/src/scrcmd_party.c"
  "${HG_UPSTREAM}/src/scrcmd_strbuf.c"
  "${HG_UPSTREAM}/src/scrcmd_sound.c" "${HG_UPSTREAM}/src/sys_flags.c" "${HG_UPSTREAM}/include/constants/sndseq.h"
  "${HG_UPSTREAM}/include/script.h" "${HG_UPSTREAM}/include/scrcmd.h" "${HG_UPSTREAM}/include/player_data.h"
  "${HG_UPSTREAM}/include/save_vars_flags.h" "${HG_UPSTREAM}/src/save_vars_flags.c" "${HG_UPSTREAM}/src/data/fieldmap/script_cmd_table.h"
  "${HG_UPSTREAM}/files/fielddata/script/scr_seq/scr_seq_0149.s" "${HG_UPSTREAM}/files/msgdata/msg/msg_0040.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_naming.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS scripts/prepare_naming.py)
add_library(hg_script STATIC "${HG_UPSTREAM}/src/script.c" src/sign_script.c src/field_script.c src/compat/pokegear.c src/compat/apricorn.c)
target_compile_definitions(hg_script PRIVATE HG_ORIGINAL_POKEGEAR="${HG_UPSTREAM}/src/save_pokegear.c" HG_ORIGINAL_APRICORN="${HG_UPSTREAM}/src/apricorn_tree_sys.c")
target_link_libraries(hg_script PUBLIC hg_field)
target_sources(hg_script PRIVATE src/compat/naming.c src/compat/field_queries.c src/compat/trainer_message.c)
target_link_libraries(hg_script PRIVATE hg_stats)
target_compile_options(hg_script PRIVATE "$<$<COMPILE_LANGUAGE:C>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/compat/script.h\">")
add_executable(test_story tests/test_story.c)
target_link_libraries(test_story PRIVATE hg_script hg_stats)
add_test(NAME original_story_runtime COMMAND test_story)
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

set(HG_YYJSON "${CMAKE_SOURCE_DIR}/build/evidence/weather/yyjson-0.12.0" CACHE PATH "Upstream-pinned yyjson with packagefile patches for original msgenc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS scripts/prepare_original_messages.py scripts/prepare_original_field_windows.py)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_original_messages.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" "${HG_YYJSON}" COMMAND_ERROR_IS_FATAL ANY)
include("${GEN}/original_message_inputs.cmake")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_pokemon.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_pokemon.py"
  "${HG_UPSTREAM}/src/pokemon.c" "${HG_UPSTREAM}/src/party.c" "${HG_UPSTREAM}/src/pm_string.c"
  "${HG_UPSTREAM}/src/string_util.c" "${HG_UPSTREAM}/src/mail.c" "${HG_UPSTREAM}/src/mail_message.c"
  "${HG_UPSTREAM}/src/move.c" "${HG_UPSTREAM}/src/seal_case.c")
add_library(hg_pokemon STATIC src/compat/pokemon.c "${HG_UPSTREAM}/src/pm_version.c")
target_compile_options(hg_pokemon PRIVATE "SHELL:-include config.h")
target_link_libraries(hg_pokemon PUBLIC hg_stats)
add_executable(test_pokemon tests/test_pokemon.c)
target_link_libraries(test_pokemon PRIVATE hg_pokemon hg_motion)
add_test(NAME original_pokemon_creation COMMAND test_pokemon)

target_link_libraries(hg_script PRIVATE hg_pokemon)
add_dependencies(reuse_audit test_pokemon)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${HG_UPSTREAM}/files/poketool/personal/personal.json" "${HG_UPSTREAM}/files/poketool/personal/personal.json.txt"
  "${HG_UPSTREAM}/files/poketool/personal/growtbl.csv" "${HG_UPSTREAM}/files/poketool/personal/wotbl.narc"
  "${HG_UPSTREAM}/files/poketool/waza/waza_tbl.narc" "${HG_UPSTREAM}/files/msgdata/msg/msg_0237.json"
  "${HG_UPSTREAM}/include/pokemon.h" "${HG_UPSTREAM}/include/pm_string.h" "${HG_UPSTREAM}/include/struct_defs/personal_narc_structs.h"
  "${HG_UPSTREAM}/include/constants/pokemon.h" "${HG_UPSTREAM}/include/constants/abilities.h"
  "${HG_UPSTREAM}/include/constants/items.h" "${HG_UPSTREAM}/include/constants/moves.h" "${HG_UPSTREAM}/tools/msgenc/charmap.txt")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${HG_UPSTREAM}/asm/unk_02017FAC.s" "${HG_UPSTREAM}/src/trainer_memo.c"
  "${HG_UPSTREAM}/src/pokedex.c" "${HG_UPSTREAM}/src/unk_02091278.c" "${HG_UPSTREAM}/include/pokedex.h")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/options.c")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${HG_UPSTREAM}/src/choose_starter.c" "${HG_UPSTREAM}/src/update_dex_received.c")
target_sources(hg_motion PRIVATE src/compat/starter.c)
add_executable(test_starter tests/test_starter.c)
target_link_libraries(test_starter PRIVATE hg_motion hg_pokemon hg_script)
add_test(NAME original_starter_task COMMAND test_starter)
add_dependencies(reuse_audit test_starter)

# Include every game compatibility archive in provider resolution. This is still
# symbol evidence, not a successful whole-game link or executed coverage.
foreach(HG_PROVIDER hg_motion hg_script hg_stats hg_pokemon hg_props hg_field_tasks hg_overlays hg_field_app hg_field_view ds_platform)
  add_dependencies(audit_native "${HG_PROVIDER}")
endforeach()

# Generated call closures grow across source files. Their recorded inputs, not
# a hand-maintained selection of roots, determine when configure must rerun.
file(READ "${GEN}/pokemon_provenance.json" HG_POKEMON_PROVENANCE)
foreach(HG_INPUT_GROUP sources resource_and_declaration_inputs)
  string(JSON HG_INPUT_COUNT LENGTH "${HG_POKEMON_PROVENANCE}" "${HG_INPUT_GROUP}")
  if(HG_INPUT_COUNT GREATER 0)
    math(EXPR HG_INPUT_LAST "${HG_INPUT_COUNT} - 1")
    foreach(HG_INPUT_INDEX RANGE 0 ${HG_INPUT_LAST})
      string(JSON HG_INPUT_PATH MEMBER "${HG_POKEMON_PROVENANCE}" "${HG_INPUT_GROUP}" ${HG_INPUT_INDEX})
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_INPUT_PATH}")
    endforeach()
  endif()
endforeach()

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_follow.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
target_sources(hg_motion PRIVATE src/compat/follow.c)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_follow.py" "${HG_UPSTREAM}/src/follow_mon.c" "${HG_UPSTREAM}/asm/unk_020658D4.s" "${HG_UPSTREAM}/files/fielddata/tsurepoke/tp_param.csv" "${HG_UPSTREAM}/files/fielddata/tsurepoke/tp_param.csv.txt")
add_executable(test_follow tests/test_follow.c)
target_link_libraries(test_follow PRIVATE hg_motion)
add_test(NAME original_follower_controller COMMAND test_follow)
add_dependencies(reuse_audit test_follow)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/include/constants/follow_mon_idx.h" "${HG_UPSTREAM}/include/follow_mon.h" "${HG_UPSTREAM}/files/fielddata/script/scr_seq/scr_seq_0003.s")

add_executable(test_prop_animation tests/test_prop_animation.c)
target_link_libraries(test_prop_animation PRIVATE hg_motion)
add_test(NAME original_prop_animation COMMAND test_prop_animation)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/include/save_misc_data.h" "${HG_UPSTREAM}/src/apricorn_tree_sys.c" "${HG_UPSTREAM}/asm/unk_020632B0.s" "${HG_UPSTREAM}/asm/overlay_01_sprite_data.s")
add_executable(test_apricorn tests/test_apricorn.c)
target_link_libraries(test_apricorn PRIVATE hg_motion)
add_test(NAME original_apricorn_state COMMAND test_apricorn)

add_executable(test_exploration tests/test_exploration.c)
target_link_libraries(test_exploration PRIVATE hg_field_app hg_motion)
add_test(NAME free_field_exploration COMMAND test_exploration)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/asm/unk_0201010C.s" "${HG_UPSTREAM}/include/screen_fade.h")
add_executable(test_screen_fade tests/test_screen_fade.c)
target_link_libraries(test_screen_fade PRIVATE hg_motion ds_platform)
add_test(NAME original_screen_fade COMMAND test_screen_fade)
add_executable(test_rest tests/test_rest.c)
target_link_libraries(test_rest PRIVATE hg_script hg_pokemon)
add_test(NAME original_rest_and_pokedex COMMAND test_rest)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/launch_application.c")

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_phone.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
target_sources(hg_script PRIVATE src/compat/phone.c)
target_link_libraries(hg_script PRIVATE hg_field_tasks hg_pokemon)
file(READ "${GEN}/phone_provenance.json" HG_PHONE_PROVENANCE)
foreach(HG_PHONE_GROUP inputs)
  string(JSON HG_PHONE_COUNT LENGTH "${HG_PHONE_PROVENANCE}" "${HG_PHONE_GROUP}")
  math(EXPR HG_PHONE_LAST "${HG_PHONE_COUNT}-1")
  foreach(HG_PHONE_INDEX RANGE 0 ${HG_PHONE_LAST})
    string(JSON HG_PHONE_PATH MEMBER "${HG_PHONE_PROVENANCE}" "${HG_PHONE_GROUP}" ${HG_PHONE_INDEX})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_PHONE_PATH}")
  endforeach()
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_phone.py" "${HG_UPSTREAM}/src/application/pokegear/phone/overlay_101_021F1D74.c" "${HG_UPSTREAM}/src/application/pokegear/phone/scripts/phone_scripts_prof_elm.c")
add_executable(test_phone tests/test_phone.c)
target_link_libraries(test_phone PRIVATE hg_script)
add_test(NAME original_phone_application COMMAND test_phone)

# Reusable sound archive renderer; all original sound data remains build-local.
set(CMAKE_CXX_STANDARD 17)
file(GLOB SSEQPLAYER_SOURCES CONFIGURE_DEPENDS "third_party/sseqplayer/*.cpp")
add_library(ds_audio STATIC src/ds_audio.cpp src/ds_audio_output.c ${SSEQPLAYER_SOURCES})
target_include_directories(ds_audio PUBLIC src PRIVATE third_party/sseqplayer "${GEN}")
configure_file("${HG_UPSTREAM}/files/data/sound/gs_sound_data.sdat" "${GEN}/resources/gs_sound_data.sdat" COPYONLY)
target_compile_definitions(ds_audio PRIVATE HG_SOUND_ARCHIVE="${GEN}/resources/gs_sound_data.sdat")
target_link_libraries(ds_audio PUBLIC "-framework AudioToolbox")
target_link_libraries(hg_script PUBLIC ds_audio)
# Sequence synthesis runs for every simulation frame, including Debug builds.
target_compile_options(ds_audio PRIVATE -O2)
add_executable(test_audio tests/test_audio.c)
target_link_libraries(test_audio PRIVATE ds_audio)
add_test(NAME original_sound_archive COMMAND test_audio)
add_executable(test_sound_channels tests/test_sound_channels.cpp)
target_link_libraries(test_sound_channels PRIVATE ds_audio)
target_include_directories(test_sound_channels PRIVATE "${GEN}")
add_test(NAME original_sound_channels COMMAND test_sound_channels)
add_executable(heartgold_audio src/audio_main.c)
target_link_libraries(heartgold_audio PRIVATE ds_audio)
add_executable(test_audio_script tests/test_audio_script.c)
target_link_libraries(test_audio_script PRIVATE hg_motion hg_script)
add_test(NAME original_script_sound_waits COMMAND test_audio_script)
add_executable(test_unlock tests/test_unlock.c)
target_link_libraries(test_unlock PRIVATE hg_script)
add_test(NAME original_unlock_notifications COMMAND test_unlock)

# Resolve boundaries against the actual transitive host-provider closure.
function(hg_collect_battle_providers task_target)
  if(NOT TARGET "${task_target}")
    return()
  endif()
  get_property(task_seen GLOBAL PROPERTY HG_BATTLE_PROVIDER_SEEN)
  if("${task_target}" IN_LIST task_seen)
    return()
  endif()
  set_property(GLOBAL APPEND PROPERTY HG_BATTLE_PROVIDER_SEEN "${task_target}")
  get_target_property(task_kind "${task_target}" TYPE)
  if(task_kind STREQUAL "STATIC_LIBRARY")
    set_property(GLOBAL APPEND PROPERTY HG_BATTLE_PROVIDER_TARGETS "${task_target}")
  endif()
  get_target_property(task_links "${task_target}" LINK_LIBRARIES)
  get_target_property(task_interfaces "${task_target}" INTERFACE_LINK_LIBRARIES)
  foreach(task_link IN LISTS task_links task_interfaces)
    if(TARGET "${task_link}")
      hg_collect_battle_providers("${task_link}")
    endif()
  endforeach()
endfunction()
set_property(GLOBAL PROPERTY HG_BATTLE_PROVIDER_SEEN "")
set_property(GLOBAL PROPERTY HG_BATTLE_PROVIDER_TARGETS "")
foreach(task_root hg_pokemon hg_motion hg_field_tasks hg_stats ds_platform)
  hg_collect_battle_providers("${task_root}")
endforeach()
get_property(HG_BATTLE_PROVIDER_TARGETS GLOBAL PROPERTY HG_BATTLE_PROVIDER_TARGETS)
list(APPEND HG_BATTLE_PROVIDER_TARGETS hg_save)
set(HG_BATTLE_PROVIDER_FILES "")
foreach(task_provider IN LISTS HG_BATTLE_PROVIDER_TARGETS)
  list(APPEND HG_BATTLE_PROVIDER_FILES "$<TARGET_FILE:${task_provider}>")
endforeach()
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_tutorial.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_tutorial.py" "${HG_UPSTREAM}/src/battle/battle_setup.c" "${HG_UPSTREAM}/include/battle/battle_setup.h" "${HG_UPSTREAM}/src/battle/battle_input.c" "${HG_UPSTREAM}/include/battle/battle_input.h" "${HG_UPSTREAM}/src/options.c" "${HG_UPSTREAM}/src/player_data.c")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_battle.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
add_custom_command(OUTPUT "${GEN}/battle_aliases.inc" "${GEN}/battle_unsupported.inc" "${GEN}/battle_boundary_decls.inc"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_battle_boundaries.py" "${GEN}" "${CMAKE_SOURCE_DIR}/src/compat/battle_providers.inc" ${HG_BATTLE_PROVIDER_FILES}
  DEPENDS ${HG_BATTLE_PROVIDER_TARGETS} "${GEN}/battle_provenance.json" scripts/prepare_battle_boundaries.py src/compat/battle_providers.inc src/compat/trainer_ai_host.inc src/compat/capture_host.inc src/compat/battle.h VERBATIM)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_trainer_ai.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS scripts/prepare_trainer_ai.py "${HG_UPSTREAM}/asm/overlay_12_battle_controller_opponent.s" "${HG_UPSTREAM}/asm/overlay_10_trainer_ai.s" "${HG_UPSTREAM}/src/battle/trainer_ai.c" "${HG_UPSTREAM}/include/battle/battle.h")
add_library(hg_battle STATIC src/compat/battle.c "${GEN}/battle_aliases.inc" "${GEN}/battle_unsupported.inc")
target_link_libraries(hg_battle PUBLIC hg_save hg_pokemon hg_stats hg_field_tasks ds_audio hg_motion)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS scripts/prepare_battle.py scripts/script_assembly.py)
add_executable(heartgold_battle_probe src/battle_main.c)
target_link_libraries(heartgold_battle_probe PRIVATE hg_battle)
add_executable(test_battle tests/test_battle.c)
target_link_libraries(test_battle PRIVATE hg_battle)
add_test(NAME original_battle_execution COMMAND test_battle)
set_tests_properties(original_battle_execution PROPERTIES TIMEOUT 10)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${HG_UPSTREAM}/src/dex_mon_measures.c" "${HG_UPSTREAM}/include/dex_mon_measures.h"
  "${HG_UPSTREAM}/files/application/zukanlist/zkn_data/zukan_data.json"
  "${HG_UPSTREAM}/files/application/zukanlist/zkn_data/zukan_data.json.txt")
add_executable(test_battle_setup tests/test_battle_setup.c)
target_link_libraries(test_battle_setup PRIVATE hg_battle)
add_test(NAME original_battle_session_setup COMMAND test_battle_setup)
set_tests_properties(original_battle_session_setup PROPERTIES TIMEOUT 10)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${HG_UPSTREAM}/src/trainer_data.c" "${HG_UPSTREAM}/include/trainer_data.h"
  "${HG_UPSTREAM}/files/poketool/trainer/trainers.json"
  "${HG_UPSTREAM}/files/poketool/trainer/trdata.json.txt"
  "${HG_UPSTREAM}/files/poketool/trainer/trpoke.json.txt"
  "${HG_UPSTREAM}/files/poketool/trmsg/trtbl.narc"
  "${HG_UPSTREAM}/files/poketool/trmsg/trtblofs.narc"
  "${HG_UPSTREAM}/files/application/custom_ball/edit/gs_cb_data.narc")
add_library(hg_battle_view STATIC src/battle_view.m)
target_compile_options(hg_battle_view PRIVATE -fobjc-arc)
target_link_libraries(hg_battle_view PUBLIC hg_battle "-framework Cocoa")
target_link_libraries(hg_field_app PRIVATE hg_battle_view)
add_executable(test_field_battle tests/test_field_battle.c)
target_link_libraries(test_field_battle PRIVATE hg_field_app hg_motion hg_battle)
add_test(NAME field_wild_battle_return COMMAND test_field_battle)
set_tests_properties(field_wild_battle_return PROPERTIES TIMEOUT 30)

add_test(NAME field_chikorita_palette COMMAND test_field_battle 152)
add_test(NAME field_shiny_chikorita_palette COMMAND test_field_battle 152 shiny)
add_test(NAME field_totodile_battle COMMAND test_field_battle 158)
set_tests_properties(field_chikorita_palette field_shiny_chikorita_palette field_totodile_battle PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
set_tests_properties(field_wild_battle_return PROPERTIES RUN_SERIAL TRUE)
add_test(NAME original_battle_sprite_palettes COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_battle_palettes.py" "${HG_UPSTREAM}" "${GEN}/resources/battle_sprites_png.narc")
add_executable(test_checkpoints tests/test_checkpoints.c)
target_link_libraries(test_checkpoints PRIVATE hg_field_app hg_motion hg_battle)
add_test(NAME connected_checkpoint_resume COMMAND test_checkpoints)
set_tests_properties(connected_checkpoint_resume PROPERTIES DEPENDS "original_background_interaction;original_catching_tutorial;original_violet_arrival" TIMEOUT 45 RUN_SERIAL TRUE WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(original_background_interaction PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_custom_target(heartgold_checkpoints COMMAND $<TARGET_FILE:test_interaction> COMMAND $<TARGET_FILE:test_tutorial> COMMAND $<TARGET_FILE:test_violet> DEPENDS test_interaction test_tutorial test_violet WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" COMMENT "Generate gameplay-produced story sources through Violet; curated teams load separately")

add_executable(test_elm_return tests/test_elm_return.c)
target_link_libraries(test_elm_return PRIVATE hg_field_app hg_motion hg_battle)
add_test(NAME original_elm_return COMMAND test_elm_return)
set_tests_properties(original_elm_return PROPERTIES DEPENDS original_background_interaction RUN_SERIAL TRUE TIMEOUT 30 WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_trainer_events tests/test_trainer_events.c)
target_link_libraries(test_trainer_events PRIVATE hg_script hg_save hg_motion)
add_test(NAME original_trainer_identity COMMAND test_trainer_events)
set_tests_properties(original_trainer_identity PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_violet tests/test_violet.c)
target_link_libraries(test_violet PRIVATE hg_field_app hg_motion hg_battle)
add_test(NAME original_violet_arrival COMMAND test_violet)
set_tests_properties(original_violet_arrival PROPERTIES DEPENDS original_catching_tutorial RUN_SERIAL TRUE TIMEOUT 45 WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_tutorial tests/test_tutorial.c)
target_link_libraries(test_tutorial PRIVATE hg_field_app hg_motion hg_battle)
add_test(NAME original_catching_tutorial COMMAND test_tutorial)
set_tests_properties(original_catching_tutorial PROPERTIES DEPENDS original_background_interaction RUN_SERIAL TRUE TIMEOUT 30 WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_target(heartgold_tutorial_check COMMAND $<TARGET_FILE:test_tutorial> --resume DEPENDS test_tutorial WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" COMMENT "Check tutorial directly from validated checkpoints")

# Full original save registry and manager. Generated original code stays local;
# the reusable flash transport owns only bounded durable backup I/O.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_save_manager.py" --upstream "${HG_UPSTREAM}" --output "${GEN}/original_save_manager.inc" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_save_c_records.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_save_records.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_bug_contest.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_field_time.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
add_library(hg_save STATIC src/compat/bug_contest.c src/compat/field_time.c src/compat/play_time.c src/compat/save_manager.c src/compat/save_records.c src/compat/save_c_records.c src/ds_save_flash.c src/ds_account.c)
target_link_libraries(hg_save PUBLIC hg_motion hg_pokemon)
target_link_libraries(test_original_field_constructor PRIVATE hg_save)
target_link_libraries(hg_script PRIVATE hg_save)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${CMAKE_SOURCE_DIR}/scripts/prepare_save_manager.py" "${CMAKE_SOURCE_DIR}/scripts/prepare_save_c_records.py"
 "${CMAKE_SOURCE_DIR}/scripts/prepare_save_records.py" "${HG_UPSTREAM}/src/save.c" "${HG_UPSTREAM}/src/save_arrays.c" "${HG_UPSTREAM}/src/unk_02018380.c")
add_executable(test_save_flash tests/test_save_flash.c src/ds_save_flash.c)
target_include_directories(test_save_flash PRIVATE src)
add_test(NAME durable_save_flash COMMAND test_save_flash)
add_executable(test_account tests/test_account.c src/ds_account.c)
target_include_directories(test_account PRIVATE src)
add_test(NAME cartridge_account_record COMMAND test_account)
add_executable(test_save_c_records tests/test_save_c_records.c)
target_link_libraries(test_save_c_records PRIVATE hg_save)
add_test(NAME original_save_c_records COMMAND test_save_c_records)
add_executable(test_save_manager tests/test_save_manager.c)
target_link_libraries(test_save_manager PRIVATE hg_save)
add_test(NAME original_save_manager COMMAND test_save_manager)
add_executable(test_save_game tests/test_save_game.c)
target_link_libraries(test_save_game PRIVATE hg_field_app hg_save)
add_test(NAME original_save_field_continue COMMAND test_save_game)
set_tests_properties(original_save_field_continue PROPERTIES DEPENDS original_background_interaction WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RUN_SERIAL TRUE)
# Track generator provenance, including newly discovered callback dependencies.
file(READ "${GEN}/save_c_records_provenance.json" HG_SAVE_C_PROVENANCE)
string(JSON HG_SAVE_C_COUNT LENGTH "${HG_SAVE_C_PROVENANCE}" functions)
if(HG_SAVE_C_COUNT GREATER 0)
  math(EXPR HG_SAVE_C_LAST "${HG_SAVE_C_COUNT} - 1")
  foreach(HG_SAVE_C_INDEX RANGE 0 ${HG_SAVE_C_LAST})
    string(JSON HG_SAVE_C_SOURCE GET "${HG_SAVE_C_PROVENANCE}" functions ${HG_SAVE_C_INDEX} source)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_SAVE_C_SOURCE}")
  endforeach()
endif()
file(READ "${GEN}/save_records_provenance.json" HG_SAVE_ASM_PROVENANCE)
string(JSON HG_SAVE_ASM_COUNT LENGTH "${HG_SAVE_ASM_PROVENANCE}" sources)
if(HG_SAVE_ASM_COUNT GREATER 0)
  math(EXPR HG_SAVE_ASM_LAST "${HG_SAVE_ASM_COUNT} - 1")
  foreach(HG_SAVE_ASM_INDEX RANGE 0 ${HG_SAVE_ASM_LAST})
    string(JSON HG_SAVE_ASM_SOURCE MEMBER "${HG_SAVE_ASM_PROVENANCE}" sources ${HG_SAVE_ASM_INDEX})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_SAVE_ASM_SOURCE}")
  endforeach()
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${HG_UPSTREAM}/include/camera.h" "${HG_UPSTREAM}/tools/msgenc/charmap.txt"
 "${HG_UPSTREAM}/files/msgdata/msg/msg_0024.json" "${HG_UPSTREAM}/include/save.h" "${HG_UPSTREAM}/include/save_arrays.h")

add_executable(test_play_time tests/test_play_time.c)
target_link_libraries(test_play_time PRIVATE hg_save)
add_test(NAME original_live_play_time COMMAND test_play_time)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_field_time.py" "${HG_UPSTREAM}/files/arc/safari_enc.json" "${HG_UPSTREAM}/files/arc/safari_enc.json.txt")
file(READ "${GEN}/field_time_provenance.json" HG_TIME_PROVENANCE)
string(JSON HG_TIME_COUNT LENGTH "${HG_TIME_PROVENANCE}" functions)
math(EXPR HG_TIME_LAST "${HG_TIME_COUNT}-1")
foreach(HG_TIME_INDEX RANGE ${HG_TIME_LAST})
 string(JSON HG_TIME_SOURCE GET "${HG_TIME_PROVENANCE}" functions ${HG_TIME_INDEX} source)
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_TIME_SOURCE}")
endforeach()
add_executable(test_field_time tests/test_field_time.c)
target_link_libraries(test_field_time PRIVATE hg_save)
add_test(NAME original_offline_field_time COMMAND test_field_time)

string(JSON HG_TIME_HEADERS LENGTH "${HG_TIME_PROVENANCE}" declaration_inputs)
math(EXPR HG_TIME_HEADER_LAST "${HG_TIME_HEADERS}-1")
foreach(HG_TIME_INDEX RANGE ${HG_TIME_HEADER_LAST})
 string(JSON HG_TIME_HEADER MEMBER "${HG_TIME_PROVENANCE}" declaration_inputs ${HG_TIME_INDEX})
 set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_TIME_HEADER}")
endforeach()
add_executable(test_save_recovery tests/test_save_recovery.c)
target_link_libraries(test_save_recovery PRIVATE hg_save)
add_test(NAME original_save_recovery COMMAND test_save_recovery)

add_executable(test_bug_contest tests/test_bug_contest.c)
target_link_libraries(test_bug_contest PRIVATE hg_save)
add_test(NAME original_bug_contest_owner COMMAND test_bug_contest)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_bug_contest.py")
file(READ "${GEN}/bug_contest_provenance.json" HG_CONTEST_PROVENANCE)
string(JSON HG_CONTEST_INPUT_COUNT LENGTH "${HG_CONTEST_PROVENANCE}" inputs)
math(EXPR HG_CONTEST_INPUT_LAST "${HG_CONTEST_INPUT_COUNT} - 1")
foreach(HG_INPUT_INDEX RANGE 0 ${HG_CONTEST_INPUT_LAST})
  string(JSON HG_INPUT_PATH MEMBER "${HG_CONTEST_PROVENANCE}" inputs ${HG_INPUT_INDEX})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/${HG_INPUT_PATH}")
endforeach()
add_executable(test_incoming_phone tests/test_incoming_phone.c)
target_link_libraries(test_incoming_phone PRIVATE hg_field_app)
add_test(NAME original_incoming_phone COMMAND test_incoming_phone)
set_tests_properties(original_incoming_phone PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" DEPENDS original_background_interaction RUN_SERIAL TRUE)
add_executable(test_contest_battle tests/test_contest_battle.c)
target_link_libraries(test_contest_battle PRIVATE hg_battle hg_save)
add_test(NAME original_contest_battle COMMAND test_contest_battle)
add_executable(test_contest_scene tests/test_contest_scene.c)
target_link_libraries(test_contest_scene PRIVATE hg_field_app)
add_test(NAME original_contest_scene COMMAND test_contest_scene)
set_tests_properties(original_contest_scene PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" DEPENDS original_background_interaction RUN_SERIAL TRUE)
add_executable(test_capture tests/test_capture.c)
target_link_libraries(test_capture PRIVATE hg_battle_view hg_save)
add_test(NAME original_wild_capture COMMAND test_capture)

add_test(NAME field_wild_capture_return COMMAND test_field_battle 155 capture)
set_tests_properties(field_wild_capture_return PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RUN_SERIAL TRUE)
foreach(HG_CAPTURE_STARTER 152 158)
  add_test(NAME field_wild_capture_${HG_CAPTURE_STARTER} COMMAND test_field_battle ${HG_CAPTURE_STARTER} capture)
  set_tests_properties(field_wild_capture_${HG_CAPTURE_STARTER} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RUN_SERIAL TRUE DEPENDS original_background_interaction)
endforeach()
set_tests_properties(field_wild_capture_return PROPERTIES DEPENDS original_background_interaction)

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_blackout.py" "${HG_UPSTREAM}" "${GEN}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_blackout.py" "${HG_UPSTREAM}/src/blackout.c")
target_sources(hg_field_app PRIVATE src/compat/blackout.c)
add_executable(test_blackout tests/test_blackout.c)
target_link_libraries(test_blackout PRIVATE hg_field_app)
add_test(NAME original_blackout_task COMMAND test_blackout)
add_executable(test_field_blackout tests/test_field_blackout.c)
target_link_libraries(test_field_blackout PRIVATE hg_field_app)
add_test(NAME field_blackout_home COMMAND test_field_blackout 1)
add_test(NAME field_blackout_center COMMAND test_field_blackout 5)
set_tests_properties(field_blackout_home field_blackout_center PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RUN_SERIAL TRUE DEPENDS original_background_interaction)

target_sources(test_field_blackout PRIVATE tests/field_ui_routes.c)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/asm/unk_0205BB1C.s")


add_executable(test_healing tests/test_healing.c)
target_link_libraries(test_healing PRIVATE hg_field_app)
add_test(NAME original_pokecenter_healing COMMAND test_healing)

foreach(species 155 158)
  add_test(NAME field_blackout_center_${species} COMMAND test_field_blackout 5 ${species})
  set_tests_properties(field_blackout_center_${species} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RUN_SERIAL TRUE DEPENDS original_background_interaction)
endforeach()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/asm/overlay_01_021E90C0.s")
add_executable(test_doors tests/test_doors.c)
target_link_libraries(test_doors PRIVATE hg_field_app)
add_test(NAME original_building_doors COMMAND test_doors)

add_executable(test_nurse tests/test_nurse.c)
target_link_libraries(test_nurse PRIVATE hg_field_app)
add_test(NAME original_nurse_interaction COMMAND test_nurse)
set_tests_properties(original_nurse_interaction PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60 DEPENDS original_background_interaction)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/asm/unk_02068FC8.s" "${HG_UPSTREAM}/asm/unk_02030A98.s" "${HG_UPSTREAM}/include/gf_rtc.h")

execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/prepare_mart.py" "${HG_UPSTREAM}" "${GEN}" "${CMAKE_C_COMPILER}" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/scripts/prepare_mart.py" "${HG_UPSTREAM}/src/scrcmd_mart.c" "${HG_UPSTREAM}/src/overlay_03/shop_menu.c" "${HG_UPSTREAM}/include/overlay_03.h" "${HG_UPSTREAM}/asm/overlay_15.s")

target_sources(hg_script PRIVATE src/compat/mart.c)

target_sources(hg_motion PRIVATE src/compat/item_use.c)

add_executable(test_mart tests/test_mart.c)
target_link_libraries(test_mart PRIVATE hg_field_app)
add_test(NAME original_pokemart_transactions COMMAND test_mart)
set_tests_properties(original_pokemart_transactions PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" DEPENDS original_background_interaction TIMEOUT 60)

target_sources(hg_script PRIVATE src/compat/delivery.c)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HG_UPSTREAM}/src/mystery_gift.c")

add_executable(test_battle_menus tests/test_battle_menus.c)
target_link_libraries(test_battle_menus PRIVATE hg_battle_view hg_save)
add_test(NAME original_battle_menus COMMAND test_battle_menus)
set_tests_properties(original_battle_menus PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" TIMEOUT 60)

# One process reuses the existing complete runtime contract tests; mutable owners
# are constructed/destroyed twice, without constructing a field application.
add_executable(test_compat_micro tests/test_compat_micro.c tests/micro_platform.c tests/micro_lifetimes.c tests/micro_scheduler.c)
target_link_libraries(test_compat_micro PRIVATE hg_save hg_overlays ds_platform)
add_test(NAME compatibility_micro COMMAND test_compat_micro)
