import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import run


class ResumeTests(unittest.TestCase):
    def test_skip_small_preserves_remaining_round(self):
        cases = [{'key': 'small', 'set': '1-small'},
                 {'key': 'medium', 'set': '2-medium'},
                 {'key': 'large', 'set': '3-large'}]
        stage, pending = run.pending_cases(cases, {'medium': 1, 'large': 2}, True)
        self.assertEqual(stage, 2)
        self.assertEqual([c['key'] for c in pending], ['medium'])

    def test_recovers_only_completed_legacy_logs(self):
        with tempfile.TemporaryDirectory() as name:
            work = Path(name)
            (work / 'logs').mkdir()
            (work / 'logs/2-medium_001-0.log').write_text('beam final 12 trials 2\n')
            (work / 'logs/2-medium_001-1.log').write_text('improved 10\n')
            completed = run.load_completed(work, True)
            self.assertEqual(completed, {'2-medium_001': 0})
            run.save_completed(work, completed)
            self.assertEqual(run.load_completed(work, True), completed)
            self.assertEqual(run.load_completed(work, False), {})

    def test_resume_schedules_medium_only_and_retains_best_programs(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name)
            work = out / 'work'
            work.mkdir()
            cases = []
            originals = {}
            for i, dataset in enumerate(run.batch.NAMES):
                key = dataset + '_001'
                folder = work / key
                folder.mkdir()
                (folder / 'input.txt').write_text('fixture')
                (folder / 'best.txt').write_text('- - - - - -\n' * (10 - i))
                originals[key] = (folder / 'best.txt').read_bytes()
                cases.append(dict(key=key, set=dataset, index=1))
            (work / 'cases.json').write_text(json.dumps(cases))
            completed = {cases[1]['key']: 0, cases[2]['key']: 1, cases[3]['key']: 1}
            run.save_completed(work, completed)
            clock = [0.0]
            calls = []

            def engine(command, stderr):
                calls.append(command)
                self.assertIn('2-medium_001', command[1])
                self.assertEqual(command[command.index('--width') + 1], '32')
                stderr.write('beam final 9 trials 1\n')
                clock[0] = 2.0
                return SimpleNamespace(returncode=0)

            with patch.multiple(run.batch, SOURCE=out, OUT=out, WORK=work), \
                 patch.object(run.batch, 'checkpoint') as checkpoint, \
                 patch.object(run.subprocess, 'run', side_effect=engine), \
                 patch.object(run.time, 'monotonic', side_effect=lambda: clock[0]), \
                 contextlib.redirect_stdout(io.StringIO()):
                run.main(['--output-dir', str(out), '--seconds', '1', '--workers', '1',
                          '--resume', '--skip-small'])
            self.assertEqual(len(calls), 1)
            state = run.load_completed(work, True)
            self.assertEqual(state[cases[1]['key']], 1)
            self.assertNotIn(cases[0]['key'], state)
            self.assertTrue(checkpoint.called)
            for key, original in originals.items():
                self.assertEqual((work / key / 'best.txt').read_bytes(), original)


if __name__ == '__main__':
    unittest.main()
