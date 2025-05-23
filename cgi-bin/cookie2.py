#!/usr/bin/env python3
import os
import cgi
from http import cookies

print("Content-Type: text/html\r\n")

# Set multiple cookies
print("Set-Cookie: test_cookie=hello_world; Path=/; Max-Age=3600\r\n")
print("Set-Cookie: session_id=abc123; Path=/; HttpOnly\r\n")
print("\r\n")  # End headers

# Get cookies from client
client_cookies = cookies.SimpleCookie()
if 'HTTP_COOKIE' in os.environ:
    client_cookies.load(os.environ['HTTP_COOKIE'])

# HTML output
print(f"""<!DOCTYPE html>
<html>
<head>
    <title>Cookie Test</title>
    <meta http-equiv="refresh" content="2">
</head>
<body>
    <h1>Cookie Test</h1>
    
    <h2>Cookies Received:</h2>
    <ul>""")

for key in client_cookies:
    print(f"        <li>{key} = {client_cookies[key].value}</li>")

print("""    </ul>
    
    <h2>Cookies Set:</h2>
    <ul>
        <li>test_cookie = hello_world</li>
        <li>session_id = abc123</li>
    </ul>
</body>
</html>""")