#!/usr/bin/env python3
"""Negative checks for the mechanism that makes original-execution claims."""
import copy,pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from field_evidence import resolve_trace
class TraceEvidence(unittest.TestCase):
    def setUp(self):
        self.events=[{'kind':'enter','offset':16},{'kind':'exit','offset':16},{'kind':'phase','name':'done','live':0}]
        self.symbols={0x1010:'original'};self.graph={'original':{}};self.manifest={'phases':[['done',0]],'required_execution':['original']}
    def resolve(self):return resolve_trace(copy.deepcopy(self.events),self.symbols,0x1000,self.graph,self.manifest)
    def test_aslr_normalization(self):self.assertEqual(self.resolve()['original'],1)
    def test_unknown_function_rejected(self):
        self.symbols[0x1010]='native_boundary'
        with self.assertRaises(ValueError):self.resolve()
    def test_unbalanced_trace_rejected(self):
        self.events.pop(1)
        with self.assertRaises(ValueError):self.resolve()
    def test_exit_without_entry_rejected(self):
        self.events.pop(0)
        with self.assertRaises(ValueError):self.resolve()
    def test_incomplete_lifecycle_rejected(self):
        self.events[-1]['live']=17
        with self.assertRaises(ValueError):self.resolve()
    def test_unobserved_function_rejected(self):
        self.manifest['required_execution'].append('not_executed')
        with self.assertRaises(ValueError):self.resolve()
if __name__=='__main__':unittest.main()
