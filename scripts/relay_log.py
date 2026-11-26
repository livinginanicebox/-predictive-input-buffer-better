import json, os, glob

parts = [f"sdk={os.environ.get('SDK_OUTCOME')} build={os.environ.get('BUILD_OUTCOME')}"]
parts.append('--- errors ---')
for f in ['configure.log', 'build.log']:
    if not os.path.exists(f):
        continue
    with open(f, encoding='utf-8', errors='replace') as fh:
        for i, line in enumerate(fh, 1):
            low = line.lower()
            if 'error' in low or 'fatal' in low:
                parts.append(f"{f}:{i}: {line.rstrip()[:400]}")
parts.append('--- build tail ---')
if os.path.exists('build.log'):
    with open('build.log', encoding='utf-8', errors='replace') as fh:
        parts.extend(l.rstrip()[:400] for l in fh.readlines()[-40:])
else:
    parts.append('(no build.log)')
body = "\n".join(parts)[:60000]
with open('payload.json', 'w', encoding='utf-8') as fh:
    json.dump({"body": body, "commit_id": os.environ['GITHUB_SHA']}, fh)
print(body[:2000])
