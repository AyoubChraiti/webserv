#!/usr/bin/env python3

import cgi
import os
from http import cookies

print("Content-Type: text/html")

# Read existing cookies
cookie = cookies.SimpleCookie(os.environ.get("HTTP_COOKIE"))
saved_name = cookie.get("name")
saved_city = cookie.get("city")

form = cgi.FieldStorage()
name = form.getvalue("name")
city = form.getvalue("city")

# If form submitted, set cookies
if name and city:
    print(f"Set-Cookie: name={name}")
    print(f"Set-Cookie: city={city}")
    saved_name = name
    saved_city = city

print()  # End of HTTP headers

# HTML Output
print("""
<!DOCTYPE html>
<html>
<head><title>Cookie Test</title></head>
<body>
<h2>Enter your name and city</h2>
<form method="post" action="/cgi-bin/cookies_test.py">
  Name: <input type="text" name="name" required><br><br>
  City: <input type="text" name="city" required><br><br>
  <input type="submit" value="Submit">
</form>
""")

if saved_name and saved_city:
    print(f"<h3>Welcome back, {saved_name.value} from {saved_city.value}!</h3>")

print("</body></html>")
