import datetime
import os

now = datetime.datetime.now().astimezone()
timestamp = now.strftime('%A %b %d, %Y %H:%M:%S %Z')

placeholder = "CREATED_TIMESTAMP"

for root, dirs, files in os.walk('.'):
    for fname in files:
        fpath = os.path.join(root, fname)
        try:
            with open(fpath, 'r') as f:
                content = f.read()
            if placeholder in content:
                with open(fpath, 'w') as f:
                    f.write(content.replace(placeholder, timestamp))
        except (UnicodeDecodeError, OSError):
            pass
