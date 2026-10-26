"""Canonical original parent bodies and typed, visibly open platform edges."""
import hashlib,json,re
from generated_files import write_text_if_changed


def adapt_protection(relative,name,body):
    if relative!='src/field/fieldmap.c':return body,[]
    spans={
      'FieldMap_Init':(
        '    u32 offset = 0xDCE6A1;',
        '        FS_LoadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '        offset += (0x4CF * DSProt_DetectDummy(&ov01_021E66A8));',
        '        offset += (0x6B * (DSProt_DetectNotEmulator(&ov01_021E66D8) == FALSE));',
        '        offset += DSProt_DetectFlashcart(&ov01_021E66B8) * 0x3A1;',
        '        FS_UnloadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '        if (offset % 3433 != 0) {\n            SysTask_CreateOnMainQueue(Task_AntipiracyRandom, NULL, 123);\n        }',
        '        if (offset % 4217 != 0) {\n            SysTask_CreateOnMainQueue(Task_AntipiracyRandom, NULL, 789);\n        }',
        '        if (ov01_021E662C() == FALSE) {\n            SysTask_CreateOnMainQueue(Task_AntipiracyMath, NULL, 1000);\n            SysTask_CreateOnMainQueue(Task_AntipiracyMath, NULL, 1400);\n        }'),
      'FieldMap_Exit':(
        '    int offset = 0x2AAACF;',
        '        FS_LoadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '        offset += 0x23B * DSProt_DetectDummy(&ov01_021E66C8);',
        '        offset += 0x18D * (DSProt_DetectNotFlashcart(&ov01_021E66DC) == FALSE);',
        '        BOOL notEmulator = DSProt_DetectNotEmulator(&ov01_021E66E0) == FALSE;\n        u32 unkVal = notEmulator * 0x8B + offset;',
        '        FS_UnloadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '        if (unkVal % 2221 != 0) {\n            SysTask_CreateOnMainQueue(Task_AntipiracyRandom, NULL, 7845);\n        }',
        '        if (unkVal % 1259 != 0) {\n            SysTask_CreateOnMainQueue(Task_AntipiracyRandom, NULL, 1245);\n        }')}.get(name,())
    records=[]
    for span in spans:
        if body.count(span)!=1:raise ValueError(('parent protection source drift',name,span))
        records.append(dict(removed=span,reason='DS cartridge/encoded executable instrumentation and punitive scheduling do not apply to native execution'))
        body=body.replace(span,''.join('\n' if c=='\n' else ' ' for c in span))
    if re.search(r'\b(?:DSProt_\w+|offset|unkVal|ds_protect|Task_Antipiracy\w+|ov01_021E662C)\b',body):
        raise ValueError(('unreviewed parent machine check',name))
    return body,records


def prepare_parent(root,out,functions,declaration):
    selection={
      'src/field/fieldmap.c':('FieldMap_Init','FieldMap_Main','FieldMap_Exit','FieldSystem_UpdateLocationToPlayerPosition','ov01_021E6364','ov01_021E6028','FieldSystem_HasNoGymmick','ov01_021E61E0','ov01_021E6214','OamManager_Init','Thunk_OamManager_Free','BgConfig_Init','ov01_021E6138','InitGraphicsAndManagers','FieldSystem_InitMapLoadManager','FetchMapObjectsToPreload','MapObjectsToPreload_GetIDs','MapObjectsToPreload_GetCount','MapObjectsToPreload_Free','ov01_021E64A4','ov01_021E6580','FieldMap_VBlankCallback'),
      'src/field_system.c':('FieldSystem_LoadFieldOverlayInternal','FieldSystem_Control','FieldSystem_IsPlayerMovementAllowed','FieldSystem_GetSaveData'),
      'src/system.c':('VBlankCB_DmaTasksFramecounter','VBlankCB_DmaOnly','Main_SetVBlankIntrCB','HBlankInterruptDisable','HBlankIntrRegsToggle','Main_SetHBlankIntrCB','CallHBlankIntrCallback'),
      'src/field_warp_tasks.c':('FieldMapChange_Set3DDisplay',),
      'src/gf_gfx_planes.c':('GfGfx_SetBanks','GfGfx_EngineATogglePlanes','GfGfx_SwapDisplay','GfGfx_DisableEngineAPlanes','GfGfx_DisableEngineBPlanes','GfGfx_EngineBTogglePlanes','GfGfx_BothDispOn'),
      'src/sprite.c':('ClearMainOAM','ClearSubOAM'),
      'src/bg_window.c':('BgConfig_Alloc',),
      'src/dialog_box.c':('FieldMessage_LoadTextPalettes',),
      'src/font.c':('LoadFontPal0','LoadFontPal1'),
      'src/gf_gfx_loader.c':('GfGfxLoader_GXLoadPal','GfGfxLoader_GXLoadPalWithSrcOffset','GfGfxLoader_GXLoadPalWithSrcOffsetInternal','GfGfxLoader_LoadFromNarc'),
      'src/gf_3d_render.c':('GF3dRender_InitSimpleManager','initializeSimple3DVramManager','GF3dRender_DeleteSimpleManager'),
      'src/gf_3d_vramman.c':('GF_3DVramMan_Create','GF_3DVramMan_Delete','GF_3DVramMan_DefaultInitializer','GF_3DVramMan_InitLinkedListTexVramManager','GF_3DVramMan_InitLinkedListPlttVramManager','GF_3DVramMan_InitFrameTexVramManager','GF_3DVramMan_InitFramePlttVramManager'),
      'src/unk_02055418.c':('sub_02055680','sub_020556A8','sub_020556C8','sub_020556FC'),
      'src/gf_rtc.c':('GF_RTC_GetTimeOfDay','IsNighttime'),
      'src/field_bgm.c':('FieldBGM_GetEffective','FieldBGM_GetForMapHeader','FieldBGM_GetOverride','FieldBGM_SetOverride','FieldBGM_PlayEffectiveForMapHeader','sub_02055198'),
      'src/sys_flags.c':('GetOverriddenMapMusic','CheckFlag99A','CheckDisabledCianwoodWaterfall','CheckSolvedLtSurgeGym','CheckBattledSnorlax','CheckRocketHideoutCleared'),
      'src/field/field_control.c':('FieldSystem_ProcessSoundplate','FieldSystem_ProcessSoundplateAtCoords','FieldSystem_SoundplateIsActive','GetLocalSoundplateID'),
      'src/field/overlay_01_02204004.c':('ov01_02204744','ov01_02204764'),
      'src/save_gymmick.c':('Save_Gymmick_GetType',),
      'src/unk_020648EC.c':('Gymmick_Init','Gymmick_Free'),
      'src/vram_transfer_manager.c':('GF_CreateVramTransferManager','GF_DestroyVramTransferManager','GF_RunVramTransferTasks','GF_GetNumPendingVramTransferTasks')}
    # Resolve complete intra-unit families from their lifecycle roots in one
    # traversal; SDK leaves stay typed native adapters or explicit open edges.
    for unit,roots in (('obj_char_transfer',('ObjCharTransfer_InitEx','ObjCharTransfer_Destroy','ObjCharTransfer_ClearBuffers')),
                       ('obj_pltt_transfer',('ObjPlttTransfer_Init','ObjPlttTransfer_Destroy','ObjPlttTransfer_Reset')),
                       ('bg_window',('BgConfig_Alloc','SetBothScreensModesAndDisable','InitBgFromTemplate','BG_ClearCharDataRange','BgClearTilemapBufferAndCommit','FreeBgTilemapBuffer','DoScheduledBgGpuUpdates','ScheduleBgTilemapBufferTransfer','ScheduleSetBgPosText'))):
        source=(root/('src/'+unit+'.c')).read_text()
        definitions={m[1]:m[0] for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',source,re.M|re.S)}
        pending=list(roots);reached=set()
        while pending:
            name=pending.pop()
            if name in reached:continue
            reached.add(name)
            pending.extend(n for n in re.findall(r'\b(\w+)\s*\(',definitions[name]) if n in definitions)
        selection['src/'+unit+'.c']=tuple(sorted(reached))
    def adapt(relative,name,body):
        body,changes=adapt_protection(relative,name,body)
        if name=='GF_DestroyVramTransferManager':
            span='    Heap_Free(sVramTransferManager->tasks);'
            if body.count(span)!=1:raise ValueError('transfer detach source drift')
            body=body.replace(span,'    hg_native_vram_detach(sVramTransferManager->tasks);\n'+span)
            changes.append(dict(reason='retire SDK borrowed task arena before native original Heap_Free',inserted='hg_native_vram_detach'))
        if name=='GF_3DVramMan_Delete':
            span='    Heap_Free(vramMan->plttWork);'
            if body.count(span)!=1:raise ValueError('texture work source drift')
            body=body.replace(span,'    hg_native_texture_pool_detach(vramMan->plttWork);\n    hg_native_texture_pool_detach(vramMan->texWork);\n'+span)
            changes.append(dict(reason='retire SDK borrowed allocator work before original native frees; rejects live allocations',inserted='hg_native_texture_pool_detach'))
        if name=='CopyOrUncompressTilemapData':
            for span in ('(u32)src','(u32)dest'):
                if body.count(span)!=1:raise ValueError('BG pointer alignment source drift')
                body=body.replace(span,span.replace('u32','uintptr_t'))
                changes.append(dict(reason='native pointer alignment test retains all native pointer bits',original=span,emitted=span.replace('u32','uintptr_t')))
        if name=='GfGfxLoader_GXLoadPalWithSrcOffsetInternal':
            spans={'    NNSG2dPaletteData *pPlttData;':'    NNSG2dPaletteData paletteView, *pPlttData = &paletteView;',
                'NNS_G2dGetUnpackedPaletteData(data, &pPlttData)':'hg_native_palette_unpack(data, pPlttData)',
                '(void *)((u32)pPlttData->pRawData + srcOffset)':'(u8 *)pPlttData->pRawData + srcOffset'}
            for before,after in spans.items():
                if body.count(before)!=1:raise ValueError(('palette unpack source drift',before))
                body=body.replace(before,after);changes.append(dict(reason='native pointer-bearing palette view stays on stack while original loader consumes it; raw NCLR bytes remain unchanged',original=before,emitted=after))
        return body,changes
    code,records=functions(root,selection,transform=adapt)
    names={n for ns in selection.values() for n in ns}
    indirect=set(re.findall(r'(?:\.|->)\s*(\w+)\s*\(',code))
    indirect.update(re.findall(r'\bGF3DVramManInitFunc\s+(\w+)',code))
    palette_callbacks={'GX_LoadBGPltt','GX_LoadOBJPltt','GX_LoadBGExtPltt','GX_LoadOBJExtPltt','GXS_LoadBGPltt','GXS_LoadOBJPltt','GXS_LoadBGExtPltt','GXS_LoadOBJExtPltt'}
    calls=palette_callbacks|set(re.findall(r'\b(\w+)\s*\(',code))-names-indirect-{'if','for','while','switch','sizeof','void','GF_ASSERT','NELEMS'}
    # Reuse only transports with an established native contract here. Original
    # game/resource owners without a canonical bridge remain rejecting edges.
    native={'Save_LocalFieldData_Get','LocalFieldData_GetWeatherType','LocalFieldData_GetCameraType','sub_0205E520','sub_0205E580','PlayerAvatar_InitMapFeatures','SysTaskQueue_RunTasks','OverlayManager_GetArgs','OverlayManager_New','HandleLoadOverlay','UnloadOverlayByID',
            'LocalFieldData_GetCurrentPosition','MapHeader_GetDayMusicId','MapHeader_GetNightMusicId','PlayerAvatar_GetState','CheckFlag99A','CheckScriptFlag','Save_VarsFlags_Get','LocalFieldData_GetMusicIdAddr','Heap_Create','Heap_Alloc','Heap_Free','Heap_Destroy','MI_CpuFill8',
            'FS_OVERLAY_ID','PlayerAvatar_GetXCoord','PlayerAvatar_GetZCoord','PlayerAvatar_GetFacingDirection',
            'Field3dObjectTaskManager_Create','Field3dObjectTaskManager_Delete','FieldSystem_TaskIsRunning',
            'Save_GetGymmickPtr','NNS_GfdInitVramTransferManager','NNS_GfdDoVramTransfer','hg_native_vram_detach','BillboardLists_Create','BillboardLists_Delete','hg_native_texture_pool_detach','MI_CpuClear32','ResetAllTextPrinters','TryStartMapScriptByType','GetNarcMemberSizeByIdPair','ReadWholeNarcMemberByIdPair','hg_native_palette_unpack','ov02_0224F864','ov02_0224F8F4','MI_CpuFill16','MI_CpuClear16','Heap_AllocAtEnd','Heap_FreeExplicit','memset','NNS_G2dInitOamManagerModule','OamManager_Create','OamManager_Free'}
    irq_bindings={'HBlankSystem_New':'canonical_hblank_new','HBlankSystem_Start':'canonical_hblank_start','HBlankSystem_Delete':'canonical_hblank_free','ov01_021FB5D4':'canonical_poison_new','ov01_021FB610':'canonical_poison_free','StopSE':'canonical_stop_se','GF_SndHandleMoveVolume':'canonical_sound_volume','PlaySE':'canonical_play_se','sub_02006088':'canonical_environment_sound','sub_02054874':'canonical_soundplate_at','Sound_SetFieldBGM':'canonical_sound_field_bgm','sub_02005CF4':'canonical_sound_field_guard','sub_02004EB4':'canonical_sound_field_scene','FieldTextureManager_Init':'canonical_field_texture_new','FieldTextureManager_LoadTexture':'canonical_field_texture_load',
        'FieldTextureManager_FreeAllSlots':'canonical_field_texture_clear','FieldTextureManager_Destroy':'canonical_field_texture_destroy',
        'AreaDataManager_GetMapTexture':'canonical_area_texture',
        'FieldDrawMapNameInfo_Create':'canonical_map_name_new','FieldDrawMapNameInfo_Destroy':'canonical_map_name_free',
        'Signpost_Init':'hg_field_signpost_new','Signpost_Free':'hg_field_signpost_delete',
        'WeatherManager_SetWeather':'canonical_weather_set', 'WeatherManager_Delete':'canonical_weather_delete',
        'WeatherManager_New':'canonical_weather_new',
        'AreaDataManager_GetAreaLightArchiveID':'canonical_area_light_archive',
        'AreaLightManager_New':'canonical_light_new','AreaLightManager_Free':'canonical_light_free',
        'AreaLightManager_UpdateActiveTemplate':'canonical_light_update',
        'FieldCamera_Create':'canonical_camera_create','FieldCamera_Delete':'canonical_camera_delete',
        'Camera_PushLookAtToNNSGlb':'canonical_camera_push','ov01_021EAD8C':'canonical_camera_update',
        'ModelAttributes_Init':'canonical_model_attributes_new',
        'FieldMap_InitModelAttributes':'canonical_model_attributes_configure',
        'ModelAttributes_Free':'canonical_model_attributes_free',
        'Fog_New':'canonical_fog_new','Fog_Free':'canonical_fog_free',
        'GX_DispOn':'hg_native_display_main_on','GXS_DispOn':'hg_native_display_sub_on',
        'OS_DisableIrq':'hg_native_irq_disable','OS_EnableIrq':'hg_native_irq_enable',
        'OS_GetIrqMask':'hg_native_irq_mask','OS_DisableIrqMask':'hg_native_irq_disable_mask',
        'OS_EnableIrqMask':'hg_native_irq_enable_mask','OS_SetIrqFunction':'hg_native_irq_set_function',
        'GX_HBlankIntr':'hg_native_irq_hblank_request','OS_SetIrqCheckFlag':'hg_native_irq_check_set','MI_WaitDma':'hg_native_irq_wait_dma'}
    irq_bindings.update({'G2_BlendNone':'hg_native_video_blend_main',
        'G2S_BlendNone':'hg_native_video_blend_sub','ResetVisibleHardwareWindows':'hg_native_video_windows_reset'})
    irq_bindings.update({'NNS_G3dInit':'hg_prop_renderer_initialize','G3X_InitMtxStack':'hg_prop_renderer_reset_matrices',
        'G3_SwapBuffers':'hg_native_swap_buffers','GX_SetVisiblePlane':'hg_native_visible_planes',
        'G2_SetBG0Priority':'hg_native_bg0_priority','G3X_SetShading':'hg_native_shading',
        'G3X_AntiAlias':'hg_native_antialias','G3X_AlphaTest':'hg_native_alpha_test',
        'G3X_AlphaBlend':'hg_native_alpha_blend','G3X_EdgeMarking':'hg_native_edge_marking',
        'G3X_SetFog':'hg_native_fog','G3X_SetClearColor':'hg_native_clear_color','G3_ViewPort':'hg_native_viewport',
        'NNS_GfdGetLnkTexVramManagerWorkSize':'hg_native_texture_work_size',
        'NNS_GfdGetLnkPlttVramManagerWorkSize':'hg_native_texture_work_size',
        'NNS_GfdInitLnkTexVramManager':'hg_native_texture_pool_init',
        'NNS_GfdInitLnkPlttVramManager':'hg_native_palette_pool_init',
        'GX_GetBankForOBJ':'hg_native_obj_bank','GX_GetBankForSubOBJ':'hg_native_sub_obj_bank',
        'GX_GetBankForOBJExtPltt':'hg_native_obj_palette_bank','GX_GetBankForSubOBJExtPltt':'hg_native_sub_obj_palette_bank',
        'GX_SetOBJVRamModeChar':'hg_native_obj_mode','GXS_SetOBJVRamModeChar':'hg_native_sub_obj_mode',
        'NNS_G2dInitImageProxy':'hg_native_image_proxy_init','NNS_G2dInitImagePaletteProxy':'hg_native_image_proxy_init',
        'NNS_G2dGetImageLocation':'hg_native_image_proxy_location',
        'GX_LoadOAM':'hg_native_oam_load_main','GXS_LoadOAM':'hg_native_oam_load_sub',
        'DC_FlushRange':'hg_native_cache_publish','GX_SetDispSelect':'hg_native_display_select'})
    irq_bindings.update({'GX_SetGraphicsMode':'hg_native_graphics_mode','GXS_SetGraphicsMode':'hg_native_sub_graphics_mode',
        'GX_SetBGCharOffset':'hg_native_bg_char_offset','GX_SetBGScrOffset':'hg_native_bg_screen_offset',
        'GXS_SetVisiblePlane':'hg_native_visible_sub_planes','GX_LoadBGPltt':'hg_native_bg_palette_main',
        'GX_LoadOBJPltt':'hg_native_obj_palette_main','GXS_LoadBGPltt':'hg_native_bg_palette_sub','GXS_LoadOBJPltt':'hg_native_obj_palette_sub',
        'MI_CpuCopy16':'MI_CpuCopy8','MI_CpuCopy32':'MI_CpuCopy8'})
    for engine,prefix in enumerate(('G2','G2S')):
        load='GXS' if engine else 'GX'
        for layer in range(4):
            target=f'hg_native_bg{engine}_{layer}'
            irq_bindings[f'{prefix}_SetBG{layer}Control'+('Text' if layer>1 else '')]=target+'_control'
            irq_bindings[f'{prefix}_SetBG{layer}Priority']=target+'_priority'
            irq_bindings[f'{prefix}_BG{layer}Mosaic']=target+'_mosaic'
            irq_bindings[f'{prefix}_SetBG{layer}Offset']=target+'_scroll'
            irq_bindings[f'{load}_LoadBG{layer}Scr']=target+'_screen'
            irq_bindings[f'{load}_LoadBG{layer}Char']=target+'_char'
    for group in ('BG','BGExtPltt','SubBG','SubBGExtPltt','OBJ','OBJExtPltt','SubOBJ','SubOBJExtPltt','Tex','TexPltt'):
        irq_bindings['GX_ResetBankFor'+group]='hg_native_video_reset_'+group
        irq_bindings['GX_SetBankFor'+group]='hg_native_video_set_'+group
    native.update({'FieldEffectManager_New','FieldEffectManager_InitAnimManagerList','ov01_021F1390','FieldEffectManager_InitRenderers','ov01_021F149C','ov01_021F14A8','ov01_021F1448','GF3dRender_InitObjFromHeader','MI_CpuClear8','MapObjectManager_SetEndMovement','PlayerAvatar_GetPositionVector','MapObjectManager_ClearFlagsBits','sub_0205F568',
                   'AreaDataManager_Alloc','AreaDataManager_GetMapPropModelFile','AreaDataManager_Load','ov01_021FB904','Field3dRenderObjManager_New','MapHeader_GetAreaDataBank','MapHeader_GetMoveModelBank','AllocAtEndAndReadWholeNarcMemberByIdPair','ov01_022041C4','ov01_0220460C','MapPropAnimationManager_Init','MapPropOneShotAnimationManager_New','MapPropManager_New','MapLoadManager_New','DynamicTerrainHeightManager_New','MapLoadManager_InitialLoad','GF_RTC_CopyTime','GF_RTC_GetTimeOfDayByHour','MI_CpuClearFast'})
    irq_bindings.update({'ov01_022057DC':'canonical_objects_prepare','FieldEffect_InitRenderObject':'canonical_effects_render_init','ov01_021F91F8':'canonical_objects_graphics','FieldEffectManager_New':'canonical_effects_new','FieldEffectManager_InitAnimManagerList':'canonical_effects_anim','ov01_021F1390':'canonical_effects_resources','FieldEffectManager_InitRenderers':'canonical_effects_register','ov01_021F149C':'canonical_effects_size','ov01_021F14A8':'canonical_effects_read','ov01_021F1448':'canonical_effects_heap_free','GF3dRender_InitObjFromHeader':'canonical_effects_model','ov01_022041C4':'canonical_pool_new','AreaDataManager_Alloc':'canonical_area_new','AreaDataManager_GetMapPropModelFile':'canonical_area_headers','ov01_021FB904':'canonical_area_list','AreaDataManager_Load':'canonical_area_load','MapLoadManager_New':'canonical_map_new','DynamicTerrainHeightManager_New':'canonical_height_new','MapLoadManager_InitialLoad':'canonical_initial_load'})
    native.update({'sub_0205F1A0','OamManager_ApplyAndResetBuffers'})
    irq_bindings.update({'ov01_021FA1D0':'canonical_graphics_pool','sub_02023910':'canonical_graphics_pool_publish'})
    native.update(irq_bindings)
    sources={p:p.read_text() for p in sorted((root/'include').rglob('*.h'))}
    sources.update({root/p:(root/p).read_text() for p in selection})
    open_decls=[];open_records=[];aliases=[];inputs={root/p for p in selection}
    for name,target in irq_bindings.items():
        aliases.extend((f'#pragma push_macro("{name}")',f'#undef {name}',f'#define {name} {target}'))
    sdk={'NNS_GfdInitFrmTexVramManager':'void NNS_GfdInitFrmTexVramManager(u16 slots, BOOL defaults)',
        'NNS_GfdInitFrmPlttVramManager':'void NNS_GfdInitFrmPlttVramManager(u32 slots, BOOL defaults)',
        'G2_BlendNone':'void G2_BlendNone(void)','G2S_BlendNone':'void G2S_BlendNone(void)'}
    sdk.update({'MI_GetUncompressedSize':'u32 MI_GetUncompressedSize(const void *data)',
        'MI_UncompressLZ8':'void MI_UncompressLZ8(const void *source,void *destination)',
        'MTX22_2DAffine':'void MTX22_2DAffine(MtxFx22 *matrix,u16 angle,fx32 x,fx32 y,int mode)'})
    for prefix in ('G2','G2S'):
        for layer in (2,3):
            for suffix in ('Affine','256x16Pltt'):
                name=f'{prefix}_SetBG{layer}Control{suffix}'
                sdk[name]=f'void {name}(int size,int area,int screen,int chars)'
            name=f'{prefix}_SetBG{layer}Affine'
            sdk[name]=f'void {name}(const MtxFx22 *matrix,int centerX,int centerY,int x,int y)'
    for name in ('GX_LoadBGExtPltt','GX_LoadOBJExtPltt','GXS_LoadBGExtPltt','GXS_LoadOBJExtPltt'):
        sdk[name]=f'void {name}(const void *source,u32 offset,u32 size)'
    for prefix in ('GX','GXS'):
        for kind in ('BG','OBJ'):
            for phase in ('Begin','End'):
                name=f'{prefix}_{phase}Load{kind}ExtPltt';sdk[name]=f'void {name}(void)'
    for name in sorted(calls-native):
        candidates=[]
        for path,text in sources.items():
            for m in re.finditer(r'^(?:static )?[A-Za-z_][\w* ]+\b'+name+r'\([^;{}]*\)(?:;| \{)',text,re.M):
                signature=m[0].removesuffix(' {').removesuffix(';').removeprefix('static ')
                candidates.append((path,signature))
        if name in sdk:signature=sdk[name];origin='native typed SDK interface; no SDK implementation claimed'
        else:
            # Prefer declarations over implementations; disagreeing signatures
            # require an explicit decision rather than silent first-match reuse.
            header=[c for c in candidates if c[0].suffix=='.h']
            chosen=header or candidates
            if not chosen:raise ValueError(('missing parent contract prototype',name))
            signatures={s for p,s in chosen}
            if len(signatures)!=1:raise ValueError(('ambiguous parent contract',name,chosen))
            path,signature=chosen[0];inputs.add(path);origin=str(path.relative_to(root))
        signature=signature.replace('enum GFPalLoadLocation','enum HGParentPalLoadLocation')
        # Some upstream prototypes omit names; C11 definitions require them.
        before,params=signature.split('(',1);params=params.removesuffix(')')
        named=[]
        for i,param in enumerate(params.split(',')):
            param=param.strip()
            if param and param!='void' and (len(param.split())==1 or not re.search(r'\b[a-z_]\w*$',param)):
                param+=f' arg_{i}'
            named.append(param)
        signature=before+'('+', '.join(named)+')'
        alias='hg_parent_open_'+name
        aliases.extend((f'#pragma push_macro("{name}")',f'#undef {name}',f'#define {name} {alias}'))
        if name == 'CommPlayerManager_ForcePosition':
            open_decls.append('static '+signature.replace(name+'(',alias+'(',1)+' { canonical_comm_force_position(); }')
            open_records.append(dict(function=name,source=origin,prototype=signature,status='original Thumb guest execution; active communication ABI pending'))
        elif name == 'MapLoadManager_TrackTarget':
            open_decls.append('static '+signature.replace(name+'(',alias+'(',1)+' { canonical_map_track_target(vec, mapLoadManager); }')
            open_records.append(dict(function=name,source=origin,prototype=signature,status='original ARM execution through shared native memory mapping'))
        elif name == 'ov01_022059AC':
            open_decls.append('static '+signature.replace(name+'(',alias+'(',1)+' { canonical_followmon_pending(fieldSystem); }')
            open_records.append(dict(function=name,source=origin,prototype=signature,status='original guest execution through canonical follower flag bridge'))
        else:
            open_decls.append('static '+signature.replace(name+'(',alias+'(',1)+' { unsupported("'+name+': canonical parent contract"); }')
            open_records.append(dict(function=name,source=origin,prototype=signature,status='unimplemented; rejects before supplying a result'))
    field=(root/'include/field_system.h').read_text()
    bg=(root/'include/bg_window.h').read_text()
    map_source=(root/'src/field/fieldmap.c').read_text()
    enum=re.search(r'^enum FieldMapInitState \{.*?^\};',map_source,re.M|re.S)[0]
    palette=re.search(r'^enum GFPalLoadLocation \{.*?^\};',bg,re.M|re.S)[0].replace('enum GFPalLoadLocation','enum HGParentPalLoadLocation',1)
    for name in ('FieldDrawMapNameInfo','FieldTextureManager','UnkStruct_020556FC','HBlankSystem','FieldSystemUnkSubCC'):
        tags=set()
        for text in sources.values():
            tags.update(re.findall(r'typedef\s+struct\s+(\w+)\s*'+name+r'\s*;',text))
            if re.search(r'typedef\s+struct\s+'+name+r'\s*\{',text):tags.add(name)
        if tags!={name}:raise ValueError(('unverified parent pointer tag',name,tags))
    window_source=(root/'asm/unk_0200FA24.s').read_text()
    window_body=re.search(r'^ResetVisibleHardwareWindows:.*?thumb_func_end ResetVisibleHardwareWindows',window_source,re.M|re.S)[0]
    if not re.search(r'ldr r3,.*?sub_020131F4.*?add r1, r0, #0.*?mov r0, #0.*?bx r3',window_body,re.S):
        raise ValueError('window reset source drift')
    windows_source=(root/'asm/unk_02012DD8.s').read_text()
    windows_body=re.search(r'^sub_020131F4:.*?thumb_func_end sub_020131F4',windows_source,re.M|re.S)[0]
    if '0xFFFF1FFF' not in windows_body or '0x04001000' not in windows_body or windows_body.count('str r0, [r3]')!=2:
        raise ValueError('window register source drift')
    banks=declaration((root/'include/gf_gfx_planes.h').read_text(),'GraphicsBanks')
    banks=re.sub(r'\bGXVRam\w+\b','int',banks)
    bank_constants={'GX_VRAM_BG_128_C':4,'GX_VRAM_BGEXTPLTT_NONE':0,'GX_VRAM_SUB_BG_32_H':128,
        'GX_VRAM_SUB_BGEXTPLTT_NONE':0,'GX_VRAM_OBJ_32_FG':96,'GX_VRAM_OBJEXTPLTT_NONE':0,
        'GX_VRAM_SUB_OBJ_16_I':256,'GX_VRAM_SUB_OBJEXTPLTT_NONE':0,'GX_VRAM_TEX_01_AB':3,
        'GX_VRAM_TEXPLTT_0123_E':16}
    obj_char=(root/'src/obj_char_transfer.c').read_text()
    obj_pltt=(root/'src/obj_pltt_transfer.c').read_text()
    obj_types='\n'.join((
        'typedef HGNativeImageProxy NNSG2dImageProxy;typedef HGNativeImageProxy NNSG2dImagePaletteProxy;',
        'typedef struct NNSG2dPaletteData NNSG2dPaletteData;',
        'typedef int NNS_G2D_VRAM_TYPE;typedef int GXOBJVRamModeChar;',
        'typedef struct ObjCharTransferTasksManager ObjCharTransferTasksManager;',
        declaration((root/'include/obj_char_transfer.h').read_text(),'ObjCharTransferTemplate'),
        declaration(obj_char,'ObjCharTransferTaskState','enum'),declaration(obj_char,'ObjCharTransferTask'),
        declaration(obj_char,'ObjCharTransferTasksManager'),declaration(obj_pltt,'ObjPlttTransferTask'),
        declaration(obj_pltt,'ObjPlttTransferTasksManager'),
        'static ObjCharTransferTasksManager *sObjCharTransferTasksManager;',
        'static ObjPlttTransferTasksManager *sObjPlttTransferTasksManager;'))
    bank_constants.update({'GX_OBJVRAMMODE_CHAR_1D_32K':0x10,'GX_OBJVRAMMODE_CHAR_1D_64K':0x100010,
        'GX_OBJVRAMMODE_CHAR_1D_128K':0x200010,'GX_OBJVRAMMODE_CHAR_1D_256K':0x300010,
        'GX_VRAM_OBJ_NONE':0,'GX_VRAM_OBJ_16_F':32,'GX_VRAM_OBJ_16_G':64,'GX_VRAM_OBJ_64_E':16,
        'GX_VRAM_OBJ_80_EF':48,'GX_VRAM_OBJ_80_EG':80,'GX_VRAM_OBJ_96_EFG':112,
        'GX_VRAM_OBJ_128_A':1,'GX_VRAM_OBJ_128_B':2,'GX_VRAM_OBJ_256_AB':3,
        'GX_VRAM_SUB_OBJ_NONE':0,'GX_VRAM_SUB_OBJ_128_D':8,
        'GX_VRAM_OBJEXTPLTT_0_F':32,'GX_VRAM_OBJEXTPLTT_0_G':64,'GX_VRAM_SUB_OBJEXTPLTT_0_I':256,
        'NNS_G2D_VRAM_TYPE_NEITHER':0,'NNS_G2D_VRAM_TYPE_2DMAIN':1,'NNS_G2D_VRAM_TYPE_2DSUB':2})
    bg_types='\n'.join((declaration(bg,'BgTemplate'),
        *(declaration(bg,n,'enum') for n in ('GFBgType','GFBgScreenSize','GFBgBufferSize','BgPosAdjustOp')),
        declaration(bg,'GFPalSlotOffset','enum').replace('enum GFPalSlotOffset','enum HGParentPalSlotOffset',1),
        declaration((root/'include/gf_gfx_planes.h').read_text(),'GFBgLayer','enum'),
        re.sub(r'\b(?:GXDispMode|GXBGMode|GXBG0As)\b','int',declaration(bg,'GraphicsModes')),
        'typedef int GXBGScrSizeText,GXBGColorMode,GXBGScrBase,GXBGCharBase,GXBGExtPltt,GXBGScrSizeAffine,GXBGAreaOver,GXBGScrSize256x16Pltt;',
        'typedef struct {fx32 m[4];} MtxFx22;'))
    bank_constants.update({'GX_DISPMODE_GRAPHICS':0x10000,'GX_BGMODE_0':0,'GX_BG0_AS_3D':8,
        'GX_BGSCROFFSET_0x00000':0,'GX_BGCHAROFFSET_0x00000':0,'GX_BG_COLORMODE_16':0,
        'GX_BG_SCRBASE_0x0000':0,'GX_BG_SCRBASE_0x0800':1,'GX_BG_SCRBASE_0x1000':2,
        'GX_BG_CHARBASE_0x08000':2,'GX_BG_CHARBASE_0x10000':4,'GX_BG_CHARBASE_0x14000':5,
        'GX_BG_EXTPLTT_01':0,'GX_BG_EXTPLTT_23':1,'GX_BG_AREAOVER_XLU':0})
    for category,sizes in (('TEXT',('256x256','512x256','256x512','512x512')),
                           ('AFFINE',('128x128','256x256','512x512','1024x1024')),
                           ('256x16PLTT',('128x128','256x256','512x512','1024x1024'))):
        bank_constants.update({f'GX_BG_SCRSIZE_{category}_{size}':i for i,size in enumerate(sizes)})
    bank_constants.update({f'GX_PLANEMASK_BG{i}':1<<i for i in range(4)})
    sys_flags_source=(root/'src/sys_flags.c').read_text()
    bg_data=[re.search(r'^struct MusicOverride \{.*?^\};',sys_flags_source,re.M|re.S)[0],re.search(r'^static const struct MusicOverride sMusicOverrideMap\[\] = \{.*?^\};',sys_flags_source,re.M|re.S)[0]]
    control=(root/'src/field/field_control.c').read_text()
    soundplate_header=(root/'include/field/field_control.h').read_text()
    bg_data.extend(declaration(soundplate_header,name) for name in ('Soundplate','SoundplateStruct'))
    bg_data.append('_Static_assert(sizeof(Soundplate)==8 && sizeof(SoundplateStruct)==4,"soundplate byte ABI");')
    soundplate_data=control[control.index('enum SoundplateSoundID {'):control.index('static int GetLocalSoundplateID(')]
    bg_data.append(soundplate_data)
    renderer_data=(root/'asm/overlay_01_data_02208BFC.s').read_text().split('defaultFieldEffectRenderers:',1)[1].split('.public ov01_02208C5C',1)[0]
    renderer_ids=re.findall(r'^\s*\.word (0x[0-9a-fA-F]+)\s*$',renderer_data,re.M)
    if len(renderer_ids)!=24 or renderer_ids[-1]!='0x17':raise ValueError('original effect renderer order drift')
    bg_data.append('int defaultFieldEffectRenderers[] = {'+','.join(renderer_ids)+'};')
    for name in ('sBgTemplate_1','sBgTemplate_2','sBgTemplate_3'):
        matches=list(re.finditer(r'^static const BgTemplate '+name+r' = \{.*?^\};',map_source,re.M|re.S))
        if len(matches)!=1:raise ValueError(('BG template source drift',name))
        bg_data.append(matches[0][0])
    extra_types='\n'.join(('#define GX_BUFFERMODE_W 1',
        'typedef struct MapPropAnimation MapPropAnimation;',
        'typedef struct NNSG3dResTex NNSG3dResTex;',
        declaration((root/'include/unk_02055418.h').read_text(),'UnkStruct_020556FC'),
        '#include "constants/field/field_effect_renderer.h"',
        'extern int defaultFieldEffectRenderers[];',
        '#define MAX_MAP_OBJECTS_TO_PRELOAD 24\n#define MAP_OBJECT_PRELOAD_SENTINEL 0xffff',
        re.search(r'typedef enum RTC_TimeOfDay \{.*?\} TIMEOFDAY;', (root/'include/gf_rtc.h').read_text(),re.S)[0],
        'TIMEOFDAY GF_RTC_GetTimeOfDayByHour(s32);',
        declaration((root/'include/field/overlay_01_02204004.h').read_text(),'FieldSystemUnkSub104_Sub8'),
        declaration((root/'include/field/overlay_01_02204004.h').read_text(),'FieldSystemUnkSub104'),
        declaration(field,'MapObjectsToPreload'),
        '#define MAX_MAP_OBJECTS_TO_PRELOAD 24\n#define MAP_OBJECT_PRELOAD_SENTINEL 0xffff',
        re.search(r'^GXBufferMode gG3dDepthBufferingMode = GX_BUFFERMODE_W;', (root/'src/camera.c').read_text(),re.M)[0].replace('GXBufferMode','static int')))
    gym_source=(root/'src/unk_020648EC.c').read_text();gym_tables=[]
    for table in ('sInitFuncs','sFreeFuncs'):
        text=re.search(r'^static const FieldSystemFunc1 '+table+r'\[\] = \{.*?^\};',gym_source,re.M|re.S)[0]
        for target in sorted(set(re.findall(r'= (Gymmick\w+)',text))):
            name='hg_parent_open_'+target
            gym_tables.append('static void '+name+'(FieldSystem *field){(void)field;unsupported("'+target+': original overlay callback");}')
            text=re.sub(r'\b'+target+r'\b',name,text)
        gym_tables.append(text)
    bg_data.extend(['typedef void (*FieldSystemFunc1)(FieldSystem *);']+gym_tables)
    types='\n'.join((extra_types,'#include "compat/native_vram.h"','void ResetAllTextPrinters(void);',obj_types,bg_types,
        declaration(bg,'Background'),declaration(bg,'BgConfig'),
        declaration((root/'include/gf_3d_vramman.h').read_text(),'GF3DVramMan'),
        'typedef void (*GF3DVramManInitFunc)(void);',
        'static GF3DVramMan *sSimple3DVramManager;', 'static u32 sEngineAPlanes,sEngineBPlanes;',
        '#define GF_3D_TEXALLOC_LNK 0\n#define GF_3D_PLTTALLOC_LNK 0',
        '#define GF_3D_MEM_BLOCK_PER_TEX 128\n#define GF_3D_TEX_SLOT_SIZE 0x20000',
        '#define GF_3D_MEM_BLOCK_PER_PLTT 256\n#define GF_3D_PLTT_SLOT_SIZE 0x2000',
        '#define HW_OAM_SIZE 0x400\n#define GX_DISP_SELECT_MAIN_SUB 0\n#define GX_DISP_SELECT_SUB_MAIN 1',
        '#define GX_SORTMODE_AUTO 0\n#define GX_BUFFERMODE_W 1\n#define GX_PLANEMASK_BG0 1',
        '#define GF_PLANE_TOGGLE_ON 1\n#define GX_SHADING_TOON 0\n#define GX_FOGBLEND_COLOR_ALPHA 0',
        '#define GX_FOGSLOPE_0x8000 0\n#define RGB_BLACK 0\n#define GF_GX_CLEARCOLORDEPTH_MAX 0x7fff',

        'typedef struct { u32 nMax,nCur;NNSGfdVramTransferTask *tasks; } GF_VramTransferManager;',
        'static GF_VramTransferManager *sVramTransferManager;',banks,'typedef struct GraphicsBanks GraphicsBanks;',
        *(f'#define {name} {value}' for name,value in bank_constants.items()),
        '#include "field/field_3d_object_task.h"','#include "system.h"',
        '#include "compat/native_irq.h"', '#include "compat/native_video.h"', '#define OS_IE_H_BLANK DS_IRQ_HBLANK\n#define OS_IE_V_BLANK DS_IRQ_VBLANK\n#define GX_DEFAULT_DMAID 3',
        'typedef struct FieldDrawMapNameInfo FieldDrawMapNameInfo;',
        'typedef struct FieldTextureManager FieldTextureManager;',
        'typedef struct UnkStruct_020556FC UnkStruct_020556FC;',
        'typedef struct HBlankSystem HBlankSystem;',
        'typedef struct FieldSystemUnkSubCC FieldSystemUnkSubCC;',
        'typedef enum { PM_LCD_TOP=0, PM_LCD_BOTTOM=1 } PMLCDTarget;',
        '#ifndef PAD_BUTTON_A\n#define PAD_BUTTON_A (1u << 0)\n#endif',
        declaration(field,'FieldSystemUnkSub4'),enum,palette))
    descriptor_source=(root/'src/field/field_3d_object_task.c').read_text()
    descriptor=re.search(r'^const OverlayManagerTemplate ov01_02206378 = \{.*?^\};',descriptor_source,re.M|re.S)[0]
    # Descriptor precedes the load body and retains its original callback names.
    emitted=types+'\n'+'\n'.join(bg_data)+'\n'+ '\n'.join(open_decls+aliases)+'\n'+code.split('#line',1)[0]+'\nextern const OverlayManagerTemplate ov01_02206378;\n'+code+'\n'+''.join(f'#pragma pop_macro("{n}")\n' for n in sorted((calls-native)|irq_bindings.keys()))
    write_text_if_changed(out/'original_field_parent.inc',emitted)
    write_text_if_changed(out/'original_field_parent_provenance.json',json.dumps(dict(functions=records,
      descriptor_sha256=hashlib.sha256(descriptor.encode()).hexdigest(),
      bg_templates={re.search(r'^static const BgTemplate (\w+)',v)[1]:hashlib.sha256(v.encode()).hexdigest() for v in bg_data if v.startswith('static const BgTemplate ')},open_contracts=open_records,
      indirect_callbacks=sorted(indirect),
      native_video_boundary=dict(blend='BLDCNT halfword reset on each engine, preserving BLDALPHA/BLDY',
          windows='native translation of original tail call and DISPCNT window bits; ARM body not claimed executed',
          original_window_sha256=hashlib.sha256(window_body.encode()).hexdigest(),
          original_window_register_sha256=hashlib.sha256(windows_body.encode()).hexdigest(),
          banks='original GraphicsBanks fields retain scalar enum selectors in native int storage; original bank policy executes',
          transfer='original GF arena allocation/count/run/destruction; SDK empty arena only; enqueue/destination transfer remains unimplemented',
          geometry='existing native affine draw state plus shared register state; original camera/projection, lighting and hardware pixel equivalence remain open',
          allocator='native opaque identities and budget intervals live in original GF3DVramMan work; cached texels are borrowed; DS packed keys and compressed allocations are not exported',
          obj='unchanged original OBJ manager records contain native location views; no ARM consumer may dereference these widened records',
          oam='original ARM owner and 0x1c SDK instances remain guest records; SDK leases resolve exact guest identity; original SDK OAM reservation, entry, affine, publication/reset execute; fast DMA and software 3D remain explicit boundaries'),
      sdk_scalar_adaptation='PMLCDTarget and PAD_BUTTON_A are native scalar selectors; SDK binary object layouts are not imported',
      native_adaptations=('GFPalLoadLocation tag is isolated from the existing scoped placeholder; original palette enumerators retained',
                          'MI_CpuClear32 binds the existing byte-clear transport; original native sizeof determines allocation spans')),indent=2)+'\n')
    inputs.update((root/'asm/unk_0200FA24.s',root/'asm/unk_02012DD8.s',root/'include/gf_gfx_planes.h',root/'include/vram_transfer_manager.h',root/'include/field_system.h',root/'include/bg_window.h',root/'src/field/field_3d_object_task.c',root/'include/field/field_3d_object_task.h'))
    inputs.update((root/'src/camera.c',root/'include/gf_rtc.h',root/'include/field/overlay_01_02204004.h'))
    from prepare_original_field_windows import generate as generate_windows
    _,window_inputs=generate_windows(root,out);inputs.update(window_inputs)
    return inputs
