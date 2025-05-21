#!/usr/bin/env python3
import time
import sys

# Print CGI headers
# print("Content-Type: text/plain\r\n\r\n")
# print()
# Ensure headers are sent immediately
sys.stdout.flush()

# Infinite loop: send a dot every second
while True:
    time.sleep(1)