#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html\r\n\r\n")

print("""
<!DOCTYPE html>
<html>
<head>
    <title>Python CGI Result</title>
    <style>
        body { font-family: monospace; background: #2f3640; color: #f5f6fa; padding: 40px; }
        .box { background: #353b48; padding: 20px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); max-width: 800px; margin: 0 auto; }
        h1 { color: #e1b12c; border-bottom: 2px solid #e1b12c; padding-bottom: 10px; margin-top: 0; }
        ul { list-style-type: none; padding-left: 0; }
        li { padding: 8px 0; border-bottom: 1px dashed #718093; display: flex; }
        strong { color: #8c7ae6; width: 200px; display: inline-block; font-weight: bold; }
        span { color: #dcdde1; }
        a { background: #e1b12c; color: #2f3640; padding: 10px 20px; text-decoration: none; border-radius: 4px; font-weight: bold; border: none; float: right; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="box">
        <h1>🐍 Python CGI Environment Variables</h1>
        <ul>
""")

for key, value in sorted(os.environ.items()):
    if key.startswith("HTTP_") or key in ("SERVER_NAME", "REQUEST_METHOD", "QUERY_STRING", "CONTENT_LENGTH", "CONTENT_TYPE", "SCRIPT_NAME", "PATH_INFO"):
        print(f"            <li><strong>{key}</strong><span>{value}</span></li>")

print("""
        </ul>
        <div style="clear: both; overflow: hidden;">
            <a href="/">« Go Back to Home Base</a>
        </div>
    </div>
</body>
</html>
""")
