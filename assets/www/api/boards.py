#!/usr/bin/env python3
"""
Lightweight board persistence API.
Boards are stored as JSON files in ./boards/ (relative to this script).

Actions (query string):
  GET  ?action=list                 → JSON array of board names
  GET  ?action=load&name=<name>     → board JSON
  POST ?action=save&name=<name>     → save board (body = JSON), returns {"ok":true}
  POST ?action=delete&name=<name>   → delete board, returns {"ok":true}
"""

import os
import sys
import json
import re
import cgi
import urllib.parse

BOARDS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'boards')
NAME_RE = re.compile(r'^[a-zA-Z0-9_\-]{1,64}$')


def safe_name(name):
    """Return name if valid, else None."""
    if not name or not NAME_RE.match(name):
        return None
    return name


def board_path(name):
    return os.path.join(BOARDS_DIR, name + '.json')


def respond(data, status='200 OK'):
    body = json.dumps(data)
    sys.stdout.write('Status: {}\r\n'.format(status))
    sys.stdout.write('Content-Type: application/json\r\n')
    sys.stdout.write('Access-Control-Allow-Origin: *\r\n')
    sys.stdout.write('\r\n')
    sys.stdout.write(body)


def error(msg, status='400 Bad Request'):
    respond({'error': msg}, status)


def main():
    os.makedirs(BOARDS_DIR, exist_ok=True)

    method = os.environ.get('REQUEST_METHOD', 'GET').upper()
    qs     = os.environ.get('QUERY_STRING', '')
    params = urllib.parse.parse_qs(qs)

    def param(key):
        return params.get(key, [None])[0]

    action = param('action') or ''

    if action == 'list':
        names = []
        if os.path.isdir(BOARDS_DIR):
            for fn in sorted(os.listdir(BOARDS_DIR)):
                if fn.endswith('.json'):
                    names.append(fn[:-5])
        respond(names)

    elif action == 'load':
        name = safe_name(param('name'))
        if not name:
            error('Invalid board name')
            return
        path = board_path(name)
        if not os.path.isfile(path):
            error('Board not found', '404 Not Found')
            return
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        respond(data)

    elif action == 'save':
        if method != 'POST':
            error('POST required', '405 Method Not Allowed')
            return
        name = safe_name(param('name'))
        if not name:
            error('Invalid board name')
            return
        length = int(os.environ.get('CONTENT_LENGTH', 0) or 0)
        if length <= 0:
            error('Empty body')
            return
        body = sys.stdin.buffer.read(length).decode('utf-8', errors='replace')
        try:
            doc = json.loads(body)
        except json.JSONDecodeError as e:
            error('Invalid JSON: ' + str(e))
            return
        with open(board_path(name), 'w', encoding='utf-8') as f:
            json.dump(doc, f, indent=2)
        respond({'ok': True, 'name': name})

    elif action == 'delete':
        if method != 'POST':
            error('POST required', '405 Method Not Allowed')
            return
        name = safe_name(param('name'))
        if not name:
            error('Invalid board name')
            return
        path = board_path(name)
        if os.path.isfile(path):
            os.remove(path)
        respond({'ok': True})

    else:
        error('Unknown action')


main()
