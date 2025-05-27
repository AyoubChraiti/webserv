#!/usr/bin/env python3

import cgi
import cgitb
import os
import urllib.parse

# Enable debugging
cgitb.enable()

# Get the query string
query_string = os.environ.get('QUERY_STRING', '')
params = urllib.parse.parse_qs(query_string)

# Print headers
print("Content-Type: text/html\n")

# HTML Response
print("<html><body>")
print("<h2>Query String Parameters</h2>")
print("<ul>")
for key, values in params.items():
    for value in values:
        print(f"<li><b>{key}</b>: {value}</li>")
print("</ul>")

# Debug: print raw query string
print("<hr>")
print(f"<p><b>Raw Query String:</b> {query_string}</p>")
print("</body></html>")
