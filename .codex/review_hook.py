import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def cpp_state(repository):
    paths = subprocess.check_output(
        ["git", "-C", str(repository), "ls-files", "-z", "--cached", "--others", "--exclude-standard"]
    ).decode("utf-8").split("\0")
    state = {}
    for path in sorted(set(paths)):
        if not re.search(r"\.(cpp|h|hpp|cc|cxx|hh|hxx)$", path):
            continue
        if re.search(r"(^|/)(build|libs|python|old[^/]*)(/|$)", path):
            continue
        source = repository / path
        if source.is_file():
            state[path] = hashlib.sha256(source.read_bytes()).hexdigest()
    return state


context = json.load(sys.stdin)
repository = Path(__file__).resolve().parent.parent
identity = re.sub(r"[^a-zA-Z0-9_-]", "_", context["session_id"] + "-" + context["turn_id"])
baseline = Path(tempfile.gettempdir()) / ("robocrap-policy-" + identity + ".json")
response = {}
if context["hook_event_name"] == "UserPromptSubmit":
    if not baseline.exists():
        baseline.write_text(json.dumps(cpp_state(repository)), encoding="utf-8")
    response = {"hookSpecificOutput": {
        "hookEventName": "UserPromptSubmit",
        "additionalContext": "For code changes, follow AGENTS.md Mandatory change procedure: choose reuse first, justify added structure before editing, and separately review the actual changes before completion."
    }}
elif context["hook_event_name"] == "Stop":
    if context.get("stop_hook_active"):
        baseline.unlink(missing_ok=True)
    elif not baseline.exists():
        response = {"decision": "block", "reason": "C++ turn baseline is unavailable. Perform the AGENTS compliance review manually for code changed in this turn; do not claim automatic verification."}
    else:
        before = json.loads(baseline.read_text(encoding="utf-8"))
        after = cpp_state(repository)
        baseline.unlink()
        changed = sorted(path for path in before.keys() | after.keys() if before.get(path) != after.get(path))
        if changed:
            files = ", ".join(changed[:20])
            response = {
                "decision": "block",
                "reason": f"C++ contents changed during this turn ({len(changed)} files: {files}). Re-read AGENTS.md Mandatory C++ style and Mandatory code reduction, then separately review this turn's changes and callers. Check signatures, result channels, ownership and added structure; fix in-scope violations and record findings/source counts. Preserve prior user edits. This hook does not parse C++ or prove compliance. Follow the user's authorized scope and existing build/test policy."
            }
print(json.dumps(response))
