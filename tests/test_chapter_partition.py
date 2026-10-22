"""Verify ownership isolation, cycle handling and real registry accounting."""
import importlib.util
import json
import unittest
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
