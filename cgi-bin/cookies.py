#!/usr/bin/env python3

import os
import cgi
import html

# Parse cookies from HTTP header
cookie_header = os.getenv("HTTP_COOKIE", "")
cookies = {}
if cookie_header:
    for cookie in cookie_header.split(";"):
        if "=" in cookie:
            name, value = cookie.strip().split("=", 1)
            cookies[name] = value

# Parse form data (for new cookie input)
form = cgi.FieldStorage()
new_key = form.getfirst("key", "")
new_value = form.getfirst("value", "")

# Start CGI headers
if new_key and new_value:
    print(f"Set-Cookie: {html.escape(new_key)}={html.escape(new_value)}; Path=/; HttpOnly")

print("Content-Type: text/html\n")

# Generate the response body
print("<html>")
print("<head><title>Cookie Manager</title></head>")
print("<body>")
print("<h1>Cookie Manager</h1>")

# Show form to input new cookie
print("""
<h2>Set a New Cookie</h2>
<form method="get">
    <label>Key: <input type="text" name="key"></label><br>
    <label>Value: <input type="text" name="value"></label><br>
    <input type="submit" value="Set Cookie">
</form>
""")

# Show incoming cookies
print("<h2>Incoming Cookies:</h2>")
if cookies:
    print("<ul>")
    for name, value in cookies.items():
        print(f"<li><strong>{html.escape(name)}:</strong> {html.escape(value)}</li>")
    print("</ul>")
else:
    print("<p>No incoming cookies.</p>")

print("</body>")
print("</html>")
