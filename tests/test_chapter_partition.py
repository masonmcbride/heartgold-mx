"""Verify ownership isolation, cycle handling and real registry accounting."""
import importlib.util
import json
import unittest
import tempfile
from unittest.mock import patch
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit_chapters', ROOT / 'scripts/audit_chapters.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class PartitionTests(unittest.TestCase):
    def graph(self, names):
        graph = audit.Graph()
        for name in names:
            graph.node(name, 'fixture')
        return graph

    def test_shared_dependency_and_private_leaves(self):
        graph = self.graph(['a', 'b', 'shared', 'leaf', 'private'])
        graph.seeds['gym_a'].add('a')
        graph.seeds['gym_b'].add('b')
        for a, b in [('a', 'shared'), ('b', 'shared'), ('shared', 'leaf'), ('a', 'private')]:
            graph.edge(a, b)
        owners, _, _ = graph.partition()
        self.assertEqual(owners, dict(a='gym_a', b='gym_b', shared='root', leaf='root', private='gym_a'))

    def test_cycle_is_one_ownership_unit(self):
        graph = self.graph(['a', 'b'])
        graph.seeds['gym_a'].add('a')
        graph.seeds['gym_b'].add('b')
        graph.edge('a', 'b')
        graph.edge('b', 'a')
        owners, groups, _ = graph.partition()
        self.assertEqual(owners, dict(a='root', b='root'))
        self.assertIn(['a', 'b'], groups)

    def test_pinned_root_closes_over_single_consumer_prerequisites(self):
        graph = self.graph(['chapter', 'service', 'leaf'])
        graph.seeds['gym_a'].add('chapter')
        graph.edge('chapter', 'service')
        graph.edge('service', 'leaf')
        graph.pinned.add('service')
        owners, _, _ = graph.partition()
        self.assertEqual(owners, dict(chapter='gym_a', service='root', leaf='root'))

    def test_missing_node_fails(self):
        graph = self.graph(['a'])
        graph.edge('a', 'missing')
        with self.assertRaises(ValueError):
            graph.partition()

    def test_closure_follows_callback_tables_cycles_and_missing_leaves(self):
        import sys
        sys.path.insert(0, str(ROOT / 'scripts'))
        import dependency_closure as closure
        context = 'src/fixture.c'
        a, table, callback = ['llvm:' + context + ':' + n for n in ('entry', 'callbacks', 'callback')]
        native = dict(functions={
            a: dict(unit=context, name='entry', calls=[], references=['callbacks'], indirect=['dispatch']),
            table: dict(unit=context, name='callbacks', kind='global', calls=[], references=['callback'], indirect=[]),
            callback: dict(unit=context, name='callback', calls=['entry', 'missing'], references=[], indirect=[]),
        }, public={}, platform=[])
        with tempfile.TemporaryDirectory() as folder:
            project = Path(folder)
            (project / 'compile_commands.json').write_text(json.dumps([dict(file=str(project / context), directory=folder, command='clang -c fixture.c -o fixture.o')]))
            with patch.object(closure, 'native_calls', return_value=native), patch.object(closure, 'source_index', return_value={}), patch.object(closure.subprocess, 'check_output', return_value=''):
                report = closure.function_closure(project, project, project, ['entry'], context, project / 'closure.json')
            self.assertFalse(report['complete'])
            self.assertEqual(set(report['edges'][a]), {table})
            self.assertEqual(set(report['edges'][table]), {callback})
            self.assertTrue(any(set(c) == {a, table, callback} for c in report['cycles']))
            self.assertEqual({e['kind'] for e in report['open_edges']}, {'indirect_native_call', 'missing_or_ambiguous_definition'})
            # A known platform leaf closes a direct path, never an indirect call.
            native['functions'][a]['indirect'] = []
            native['platform'] = ['missing']
            with patch.object(closure, 'native_calls', return_value=native), patch.object(closure, 'source_index', return_value={}), patch.object(closure.subprocess, 'check_output', return_value=''):
                report = closure.function_closure(project, project, project, ['entry'], context, project / 'closure.json')
            self.assertTrue(report['complete'])

    def test_source_closure_indexes_original_assembly_callback_tables(self):
        import sys
        sys.path.insert(0, str(ROOT / 'scripts'))
        import dependency_closure as closure
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'upstream/game'
            (root / 'asm').mkdir(parents=True)
            (root / 'asm/callbacks.s').write_text('callbacks:\n\t.word callback\n\t.word 0\n\tthumb_func_start callback\ncallback:\n\tbx lr\n\tthumb_func_end callback\n')
            cache = Path(folder) / 'build/audit/source-index.json'
            first = closure.source_index(root, cache)
            self.assertEqual(first['callbacks'][0]['kind'], 'table')
            self.assertIn('callback', first['callbacks'][0]['body'])
            self.assertEqual(first['callback'][0]['kind'], 'assembly')
            self.assertEqual(closure.source_index(root, cache), first)
            with (root / 'asm/callbacks.s').open('a') as stream:
                stream.write('later:\n\t.word callback\n')
            self.assertIn('later', closure.source_index(root, cache))

    def test_assembly_calls_exclude_comments_and_follow_tail_branches(self):
        import sys
        sys.path.insert(0, str(ROOT / 'scripts'))
        import dependency_closure as closure
        row = dict(kind='assembly', source='asm/fixture.s', body='entry: ; pretend_call(comment_only)\n add r0, r1, #0 ; callback(word)\n bl direct\n bne conditional\n b local\nlocal:\n ldr r3, pointer\n bx r3\n bx lr\npointer: .word callback\n')
        calls, refs, indirect = closure.original_dependencies(row, dict(callback=[], comment_only=[], word=[]))
        self.assertEqual(calls, {'direct', 'conditional'})
        self.assertEqual(refs, {'callback'})
        self.assertEqual(indirect, {'r3'})
        # Data rows refer to functions but cannot contain executable C calls.
        calls, refs, indirect = closure.original_dependencies(dict(kind='table', source='asm/fixture.s', body='.word callback ; add(fake)\n'), dict(callback=[]))
        self.assertEqual((calls, refs, indirect), (set(), {'callback'}, set()))

    def test_llvm_references_ignore_encoded_resource_bytes(self):
        import sys
        sys.path.insert(0, str(ROOT / 'scripts'))
        import dependency_closure as closure
        self.assertEqual(closure.llvm_references('private constant [8 x i8] c"@fake\\00", ptr @callback'), {'callback'})
        self.assertEqual(closure.llvm_references('[2 x ptr] [ptr @one, ptr @two]'), {'one', 'two'})

    def test_original_source_partition(self):
        # audit_save is an existing script-side parser imported by build().
        import sys
        sys.path.insert(0, str(ROOT / 'scripts'))
        manifest = json.loads((ROOT / 'scripts/johto_chapters.json').read_text())
        report = audit.build(ROOT / 'upstream/pokeheartgold', manifest)
        nodes = report['nodes']
        self.assertEqual(len([n for n in nodes.values() if n['kind'] == 'save_record']), 48)
        self.assertEqual(len(report['map_owners']) + len(report['excluded_maps']), 540)
        self.assertEqual(set(report['worklists']), {'root', *(c['id'] for c in manifest['chapters'])})
        self.assertNotIn('unclaimed', {n['owner'] for n in nodes.values()})
        for user, dependency in report['implementation_edges']:
            self.assertIn(nodes[dependency]['owner'], ('root', nodes[user]['owner']))
        for node in nodes.values():
            if node['kind'] in ('original_handler', 'root_contract', 'save_record'):
                self.assertEqual(node['owner'], 'root')
        self.assertTrue(report['missing_evidence'])
        self.assertNotEqual(report['readiness'], 'ready')


if __name__ == '__main__':
    unittest.main()
