#!/usr/bin/env python3
"""
A simple CGI script to display the body of a POST request.

This script attempts to read the body from stdin, regardless of CONTENT_LENGTH being set.
If CONTENT_LENGTH is not available, it reads until EOF.
"""
import os
import sys

def main():
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        post_body = sys.stdin.read(content_length) if content_length > 0 else sys.stdin.read()
    except (ValueError, TypeError):
        post_body = sys.stdin.read()

    # Output HTTP headers followed by the body
    print("Content-Type: text/plain\r\n\r\n")
    print()
    print(post_body)

if __name__ == '__main__':
    main()