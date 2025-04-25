#!/usr/bin/env python3

import cgi
import cgitb
cgitb.enable()  # Show errors in the browser

# Required response headers
# print("Content-Type: text/html\n")

# Get the POST data
form = cgi.FieldStorage()

# Extract individual fields
name = form.getvalue("name")
email = form.getvalue("email")

# Return the response
print(f"<html><body>")
print(f"<h2>Received POST data:</h2>")
print(f"<p>Name: {name}</p>")
print(f"<p>Email: {email}</p>")
print(f"</body></html>")
