"""Guard original-owner discovery against host substitution and lost callbacks."""
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import json
import re

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from audit_original_field import inventory, cross_reference_frontier, add_runtime_evidence, main as audit_main
from dependency_closure import source_index, original_dependencies, function_closure
import dependency_closure
from prepare_original_field import declaration, prepare, functions, native_field_initialization
from prepare_original_sound import write_state_inventory


class OriginalFieldAuditTests(unittest.TestCase):
    def test_compiler_assembly_preserves_semicolon_data(self):
        from guest_assembly import assemble
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            source='.set hg_compiler_syntax, 1\n.text\n.global entry\n.thumb_func\nentry:\n bx lr\n.section .rodata\n.ascii "a;b"\n.set hg_compiler_syntax, 0\n'
            image,code,imports=assemble(source,root,root,Path('/usr/bin/clang'),'semicolon','SEMICOLON')
            self.assertEqual(code,2)
            self.assertIn(b'a;b',image)
            self.assertFalse(imports)

    def test_sound_rollback_inventory_requires_all_static_identities_and_sizes(self):
        with tempfile.TemporaryDirectory() as folder:
            out=Path(folder)
            source='.type work,%object\n.comm work,84,4\n.type level,%object\n.size level,4\n'
            write_state_inventory(out,source+'.type .Lsound_.str,%object\n')
            text=(out/'original_sound_state.inc').read_text()
            self.assertIn('HG_SOUND_STATIC_BYTES 88',text)
            self.assertIn('{HG_LOADER_work,84}',text)
            self.assertIn('{HG_LOADER_level,4}',text)
            with self.assertRaises(ValueError):write_state_inventory(out,source+'.type boot.initialized,%object\n')
            with self.assertRaises(ValueError):write_state_inventory(out,source+'.type unsized,%object\n')

    def test_frontier_evidence_does_not_substitute_native_spelling_or_exports(self):
        with tempfile.TemporaryDirectory() as folder:
            project=Path(folder);(project/'src').mkdir();generated=project/'generated';generated.mkdir()
            (project/'src/provider.c').write_text('void missing(void) {}\n')
            (generated/'original_field_parent_provenance.json').write_text(json.dumps(dict(
                functions=[dict(source='src/owner.c',function='entry'),dict(source='asm/callback.s',function='callback')],
                open_contracts=[dict(function='missing',status='unimplemented; rejects before supplying a result')],
            )))
            (generated/'original_loader_guest.inc').write_text('#define HG_LOADER_CODE_SIZE 32u\n#define HG_LOADER_callback 33554432u\n#define HG_LOADER_NNS_G2dInitRenderer 33554434u\n{34603008u,"missing"},\n')
            definitions={
                'entry':[dict(kind='function',source='src/owner.c',line=1,
                    body='void entry(void) { if (flag) callback(); missing(); NNS_G2dInitRenderer(); owner->exec(); }')],
                'callback':[dict(kind='assembly',source='asm/callback.s',line=1,
                    body='callback:\n ldr r0, =0x04000060\n bx lr\n')],
            }
            report=cross_reference_frontier(inventory(definitions,{'init':('entry',)}),definitions,generated,project)
            missing=report['nodes']['missing:missing']
            self.assertEqual(missing['implementation_status'],'generated_rejecting_boundary')
            sdk=next(n for n in report['nodes'].values() if n.get('symbol')=='NNS_G2dInitRenderer')
            self.assertEqual(sdk['implementation_status'],'guest_image_export')
            self.assertEqual(sdk['reachability'],'unresolved')
            self.assertEqual(missing['implementation_evidence']['native_candidates'],['src/provider.c'])
            callback=next(n for n in report['nodes'].values() if n.get('symbol')=='callback')
            self.assertEqual(callback['implementation_status'],'guest_image_export')
            self.assertEqual(callback['reachability'],'conditionally_reachable')
            self.assertTrue(callback['callers'])
            self.assertTrue(report['hardware_operations'])
            self.assertFalse(report['complete'])
            self.assertTrue(any(n['implementation_status']=='unresolved_dispatch' for n in report['nodes'].values()))
            trace=project/'trace.log'
            trace.write_text('prefix HG_ORIGINAL_TRACE '+json.dumps(dict(event='return',entry=33554432,result=4))+'\n')
            add_runtime_evidence(report,generated,trace)
            self.assertEqual(callback['runtime_evidence'],{'return_result_4':1})
            self.assertEqual(callback['reachability'],'conditionally_reachable')
            self.assertFalse(report['complete'])

    def test_refresh_reuses_original_graph_without_rediscovery(self):
        definitions={'entry':[dict(kind='function',source='src/owner.c',line=1,body='void entry(void) { missing(); }')]}
        graph=inventory(definitions,{'initialization':('entry',)})
        with tempfile.TemporaryDirectory() as folder:
            base=Path(folder)/'before.json';output=Path(folder)/'after.json';base.write_text(json.dumps(graph))
            args=['audit','--frontier','--refresh-frontier',str(base),'--output',str(output),'--generated',str(Path(folder)/'generated')]
            with patch.object(sys,'argv',args),patch('audit_original_field.source_index',return_value=definitions),patch('audit_original_field.inventory',side_effect=AssertionError('must not rediscover')):
                audit_main()
            refreshed=json.loads(output.read_text())
            self.assertEqual(set(graph['nodes']),set(refreshed['nodes']))
            self.assertEqual(graph['edges'],refreshed['edges'])
            self.assertEqual(refreshed['refreshed_from'],str(base))
            self.assertFalse(refreshed['complete'])

    def test_guest_image_must_not_overlap_import_identities(self):
        from guest_assembly import assemble
        root=Path(__file__).resolve().parents[1]/'upstream/pokeheartgold'
        source=".text\nthumb_func_start entry\nentry:\n bl missing\n bx lr\nthumb_func_end entry\n.bss\n.space 0x100000\n"
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaisesRegex(ValueError,'guest image overlaps import identities'):
                assemble(source,root,Path(folder),Path('/usr/bin/clang'),'large','LARGE')

    def test_guest_function_and_data_import_abi(self):
        from guest_assembly import assemble
        import struct
        root=Path(__file__).resolve().parents[1]/'upstream/pokeheartgold'
        source=".text\nthumb_func_start entry\nentry:\n ldr r0, =data_slot\n ldr r1, =callback\n bx lr\n .ltorg\nthumb_func_end entry\n.rodata\nprivate.value:\n.word 0\n"
        with tempfile.TemporaryDirectory() as folder:
            image,code,imports=assemble(source,root,Path(folder),Path('/usr/bin/clang'),'tiny','TINY',data_imports=('data_slot',))
            self.assertIn(struct.pack('<I',imports['data_slot']),image)
            self.assertIn(struct.pack('<I',imports['callback']+1),image)
            self.assertNotIn('#define TINY_private.value',(Path(folder)/'tiny.inc').read_text())
            with self.assertRaisesRegex(ValueError,'outside executable section'):
                assemble(source+'.rodata\nthumb_func_start misplaced\nmisplaced:\n bx lr\nthumb_func_end misplaced\n',root,Path(folder),Path('/usr/bin/clang'),'bad','BAD',data_imports=('data_slot',))

    def test_native_protection_adaptation_is_exact_and_fails_on_source_drift(self):
        root=Path(__file__).resolve().parents[1]/'upstream/pokeheartgold'
        raw,_=functions(root,{'src/overlay_124.c':('FieldSystem_Init',)})
        # Use the actual function body, excluding its generated prototype/#line.
        body=raw[raw.index('void FieldSystem_Init(',raw.index('#line')):]
        emitted,changes=native_field_initialization('src/overlay_124.c','FieldSystem_Init',body)
        self.assertEqual(emitted.count('\n'),body.count('\n'))
        self.assertEqual(len(changes),8)
        self.assertNotIn('DSProt_',emitted)
        self.assertIn('Field_AllocateMapEvents(fieldSystem, HEAP_ID_FIELD2);',emitted)
        self.assertIn('fieldSystem->phoneRingManager = GearPhoneRingManager_New',emitted)
        with self.assertRaises(ValueError):
            native_field_initialization('src/overlay_124.c','FieldSystem_Init',body.replace('2441 * 4073','2442 * 4073'))
        with self.assertRaises(ValueError):
            native_field_initialization('src/overlay_124.c','FieldSystem_Init',body+'\nDSProt_Unknown();\n')
    def test_native_context_keeps_missing_original_literal_open(self):
        context='src/fixture.c'
        native=dict(functions={'llvm:'+context+':entry':dict(unit=context,name='entry',calls=['original'],references=[],indirect=[])},public={},platform=[])
        definitions={'original':[dict(kind='assembly',source='asm/original.s',line=1,body='entry:\n .word missing_table\n bx lr\n')]}
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            (root/'compile_commands.json').write_text(json.dumps([dict(file=str(root/context),directory=folder,command='clang -c fixture.c -o fixture.o')]))
            with patch.object(dependency_closure,'native_calls',return_value=native), patch.object(dependency_closure,'source_index',return_value=definitions), patch.object(dependency_closure.subprocess,'check_output',return_value=''):
                report=function_closure(root,root,root,['entry'],context,root/'closure.json')
            self.assertFalse(report['complete'])
            self.assertTrue(any(e['target']=='missing_table' for e in report['open_edges']))

    def test_inline_assembly_label_never_resolves_foreign_overlay_data(self):
        row=dict(kind='assembly',source='asm/overlay_a.s',body='entry:\n blne callback\n bls _0223B420\n_0223B420:\n ldr r0, literal\n bx lr\nliteral: .word callback\n')
        calls,refs,indirect=original_dependencies(row,{'_0223B420': [],'callback': []})
        self.assertEqual(calls,{'callback'})
        self.assertEqual(refs,{'callback'})
        self.assertFalse(indirect)
        row['body']='entry:\n ldr pc, [r3]\n .word missing_table\n'
        calls,refs,indirect=original_dependencies(row,{})
        self.assertEqual(refs,{'missing_table'})
        self.assertEqual(indirect,{'[r3]'})
    def test_scalar_descriptor_cycle_and_outgoing_missing_dependency(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'upstream/game'
            (root / 'src').mkdir(parents=True)
            (root / 'src/owner.c').write_text(
                'const Template descriptor = { .exec = callback };\n'
                'void entry(void) { launch(&descriptor); }\n'
                'void callback(void) { entry(); missing(); }\n')
            # A same-named host symbol must never fill the original missing leaf.
            (Path(folder) / 'src').mkdir()
            (Path(folder) / 'src/host.c').write_text('void missing(void) {}\n')
            definitions = source_index(root, Path(folder) / 'build/audit/index.json', include_host=False)
            self.assertNotIn('missing', definitions)
            report = inventory(definitions, {'field': ('entry',)})
            cycle = next(c for c in report['cycles'] if any(':entry:' in k for k in c))
            self.assertTrue(any(':descriptor:' in k for k in cycle))
            self.assertTrue(any(':callback:' in k for k in cycle))
            self.assertTrue(any(e['target'] == 'missing' for e in report['open_edges']))
            self.assertFalse(report['complete'])

    def test_platform_cut_is_explicitly_unverified(self):
        definitions = {'entry': [dict(kind='function', source='src/owner.c', line=1,
                                     body='void entry(void) { Heap_Alloc(1, 4); }')]}
        report = inventory(definitions, {'field': ('entry',)}, bounded=True)
        self.assertEqual(report['nodes']['contract:Heap_Alloc']['kind'], 'unverified_platform_contract')
        self.assertFalse(report['complete'])

    def test_projection_rejects_ambiguous_original_declaration(self):
        text = 'struct FieldSystem {\n    void *owner;\n};\n'
        self.assertEqual(declaration(text, 'FieldSystem'), text.rstrip())
        with self.assertRaises(ValueError):
            declaration(text + text, 'FieldSystem')

    def test_terrain_projection_retains_original_modes_and_branches(self):
        root = Path(__file__).resolve().parents[1]/'upstream/pokeheartgold'
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder)
            prepare(root, out)
            code = (out/'original_field_terrain.inc').read_text()
            self.assertEqual(declaration(code, 'MapLoadMode'),
                             declaration((root/'include/script.h').read_text(), 'MapLoadMode'))
            self.assertEqual(declaration(code, 'TerrainAttributes'),
                             declaration((root/'include/terrain_attributes.h').read_text(), 'TerrainAttributes'))
            records = json.loads((out/'original_field_terrain_provenance.json').read_text())
            import hashlib
            for row in records['functions']:
                emitted, expected = functions(root, {row['source']: (row['function'],)})
                self.assertEqual(expected[0]['sha256'], row['sha256'])
                self.assertIn(emitted[emitted.index('#line'):].rstrip(), code)
                self.assertNotIn('adaptations', row)
            # These branches cannot disappear merely because bedroom mode avoids them.
            self.assertIn('TerrainAttributes_New(fieldSystem, fieldSystem->mapLoadMode->separateTerrainAttributesBlockCount);', code)
            self.assertIn('PlaceSafariZoneAreas(fieldSystem->mapMatrix, fieldSystem->saveData);', code)
            self.assertIn('fieldSystem->mapLoadType = MAP_LOAD_TYPE_BATTLE_TOWER;', code)
            self.assertIn('void sub_0205489C(void **, BOOL);', code)

    def test_parent_machine_adaptation_keeps_lifecycle_and_rejects_drift(self):
        from prepare_original_parent import adapt_protection
        root=Path(__file__).resolve().parents[1]/'upstream/pokeheartgold'
        source=(root/'src/field/fieldmap.c').read_text()
        for name in ('FieldMap_Init','FieldMap_Exit'):
            body=re.search(r'^BOOL '+name+r'\([^;{}]*\) \{.*?^\}',source,re.M|re.S)[0]
            emitted,records=adapt_protection('src/field/fieldmap.c',name,body)
            expected=body
            for record in records:
                span=record['removed']
                expected=expected.replace(span,''.join('\n' if c=='\n' else ' ' for c in span))
            self.assertEqual(expected,emitted)
            self.assertIn('switch (*state)',emitted)
            self.assertNotIn('DSProt_',emitted)
            with self.assertRaises(ValueError):
                adapt_protection('src/field/fieldmap.c',name,body.replace('FS_LoadOverlay(', 'FS_ChangedOverlay(',1))
        body=re.search(r'^BOOL FieldMap_Main\([^;{}]*\) \{.*?^\}',source,re.M|re.S)[0]
        self.assertEqual((body,[]),adapt_protection('src/field/fieldmap.c','FieldMap_Main',body))

    def test_generated_owner_preserves_every_original_field(self):
        root = Path(__file__).resolve().parents[1] / 'upstream/pokeheartgold'
        original = (root / 'include/field_system.h').read_text()
        with tempfile.TemporaryDirectory() as folder:
            prepare(root, Path(folder))
            header = (Path(folder) / 'original_field_owner.h').read_text()
            for name in ('FieldSystem', 'FollowMon', 'FieldProcessManager', 'FieldInput'):
                self.assertEqual(declaration(header, name), declaration(original, name))
            parent=(Path(folder)/'original_field_parent.inc').read_text()
            self.assertEqual(declaration(parent,'FieldSystemUnkSub4'),declaration(original,'FieldSystemUnkSub4'))
            provenance=json.loads((Path(folder)/'original_field_parent_provenance.json').read_text())
            self.assertIn('hBlankIntr',provenance['indirect_callbacks'])
            open_names={row['function'] for row in provenance['open_contracts']}
            self.assertNotIn('hBlankIntr',open_names)
            self.assertNotIn('OS_DisableIrq',open_names)
            for resolved in ('G2_BlendNone','G2S_BlendNone','ResetVisibleHardwareWindows','GfGfx_SetBanks','GF_CreateVramTransferManager','BillboardLists_Create'):
                self.assertNotIn(resolved,open_names)
            self.assertNotIn('GF3dRender_InitSimpleManager',open_names)
            for name in ('BgConfig_Init','InitGraphicsAndManagers','FieldSystem_InitMapLoadManager','FetchMapObjectsToPreload','AreaDataManager_Load'):
                self.assertNotIn(name,open_names)
            self.assertNotIn('ov01_021E64A4',open_names)
            self.assertNotIn('FieldEffectManager_New',open_names)
            self.assertNotIn('ov01_021E6580',open_names)
            self.assertNotIn('FieldMap_InitModelAttributes',open_names)
            self.assertNotIn('ModelAttributes_Init',open_names)
            self.assertNotIn('Fog_New',open_names)
            self.assertNotIn('FieldCamera_Create',open_names)
            self.assertNotIn('AreaLightManager_New',open_names)
            self.assertNotIn('WeatherManager_New',open_names)
            for name in ('GF_3DVramMan_Create','GF3dRender_InitSimpleManager','initializeSimple3DVramManager','ObjCharTransfer_InitEx','ObjPlttTransfer_Init','BgConfig_Alloc'):
                row=next(row for row in provenance['functions'] if row['function']==name)
                emitted,records=functions(root,{row['source']:(name,)})
                self.assertEqual(records[0]['sha256'],row['sha256'])
                self.assertIn(emitted[emitted.index('#line'):].rstrip(),parent)
            self.assertIn('enqueue/destination transfer remains unimplemented',provenance['native_video_boundary']['transfer'])
            for name in ('HBlankIntrRegsToggle','Main_SetHBlankIntrCB','CallHBlankIntrCallback'):
                emitted,records=functions(root,{'src/system.c':(name,)})
                row=next(row for row in provenance['functions'] if row['function']==name)
                self.assertEqual(records[0]['sha256'],row['sha256'])
                self.assertIn(emitted[emitted.index('#line'):].rstrip(),parent)
            # Re-running generation preserves content and modification times.
            before = {p.name: p.stat().st_mtime_ns for p in Path(folder).iterdir()}
            prepare(root, Path(folder))
            self.assertEqual(before, {p.name: p.stat().st_mtime_ns for p in Path(folder).iterdir()})


if __name__ == '__main__':
    unittest.main()
