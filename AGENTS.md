# UFUG2106 project workspace

This file is the handoff entry point for coding agents, including OpenCode.
Read `docs/WORKSPACE_HANDOFF.md` and `docs/PROGRESS_20261110.md` before making changes.

## Project boundaries

- Build a single-threaded CPU program with exactly two CLI arguments: input and output paths.
- Match the truth table on every assignment and emit only the permitted AND2/OR2/NOT format.
- Preserve the instructor's original files in `official/`; hashes are in `official/SHA256SUMS.json`.
- Keep `releases/stage1_candidate/` unchanged. It is the frozen technical candidate, not proof of submission.
- `src/baseline.cpp` is an ablation, not automatically the method to compare against in Stage 2.
- Never overwrite historical measured results. Use a fresh results directory and record source/binary hashes.
- Keep generated binaries, raw run snapshots and delivery archives out of commits unless explicitly requested.
- Record actual AI assistance and actual teammate contributions; do not invent reviews, experiments or submissions.
- The repository includes the existing course AI-policy notes. Preserve provenance and consult the current
  official instructions when assessing eligibility; do not present this artifact as independently student-written.

## Working commands

```bash
make
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/evaluate_release.py --output results/a-new-run
```

Use system `python3` on this machine: it has NumPy. The separate Canvas-agent virtualenv does not.
For production algorithm changes, run generated regression cases and the complete official/independent
verification. For documentation-only edits, verify links/paths instead of rerunning expensive experiments.
Do not normalize evaluator CSV line endings: retained manifests hash their original bytes.

## Repository

Remote: https://github.com/zdu881/UFUG2106-Project (private).
The real workspace is `/home/nyz/2106Proj`; the former Canvas project path is a compatibility symlink.
Do not modify the surrounding Canvas-monitor repository as part of this project.
