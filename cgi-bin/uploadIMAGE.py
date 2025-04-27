#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
cgitb.enable()  # Enable error reporting for debugging

def main():
    # Print minimal HTTP headers
    print("Content-Type: text/plain; charset=utf-8")
    print()
    
    # Get request method
    method = os.environ.get("REQUEST_METHOD", "GET")
    print(f"Request Method: {method}\n")

    if method == "POST":
        # Get content length
        try:
            content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        except ValueError:
            content_length = 0
            
        print(f"Content-Length: {content_length} bytes")
        print(f"Content-Type: {os.environ.get('CONTENT_TYPE', 'unknown')}\n")

        # Read binary data from stdin
        if content_length > 0:
            try:
                data = sys.stdin.buffer.read(content_length)
                print(f"Successfully read {len(data)} bytes")
                print("\nFirst 16 bytes (hex):")
                print(" ".join(f"{b:02x}" for b in data[:16]))
                
                # Optional: Save received data to a file
                with open("uploaded_file.bin", "wb") as f:
                    f.write(data)
                print("\nFile saved as uploaded_file.bin")
                
            except Exception as e:
                print(f"\nError reading input: {str(e)}")
        else:
            print("No content received")
    else:
        print("Send a POST request with binary data to test")

if __name__ == "__main__":
    main()