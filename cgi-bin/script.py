#!/usr/bin/env python3

import cgi
import cgitb
import os

# Enable CGI traceback for debugging
cgitb.enable()

# Generate HTML content
html_content = """\
<html>
<head><title>GET Request Test</title></head>
<body>
<h1>GET Request CGI Test</h1>
"""

# Get query parameters from GET request
form = cgi.FieldStorage()
param1 = form.getvalue("param1", "default_value1")
param2 = form.getvalue("param2", "default_value2")

html_content += f"<p>Parameter 1: {param1}</p>"
html_content += f"<p>Parameter 2: {param2}</p>"
html_content += "<p>Try adding ?param1=value1&param2=value2 to the URL</p>"
html_content += "</body></html>"

# Compute content length
content_length = len(html_content.encode('utf-8'))

# Print headers
print("Content-Type: text/html; charset=utf-8")
print(f"Content-Length: {content_length}")
print()  # Empty line to separate headers from body

# Print content
print(html_content)