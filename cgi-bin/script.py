#!/usr/bin/env python3
import cgi
import sys

def main():
    # HTTP Headers
    print("Content-Type: text/html; charset=utf-8")  # HTML response
    print()  # End of headers

    # HTML Content
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Python CGI Response</title>
        <style>
            body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
            h1 { color: #2c3e50; }
            .message { font-size: 1.5em; color: #3498db; }
        </style>
    </head>
    <body>
        <h1>Python CGI Script</h1>
        <p class="message">Hello, I am a CGI script!</p>
        <p>Your request was processed successfully.</p>
    </body>
    </html>
    """
    print(html_content)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        # Error handling (logs to server error log)
        print("Content-Type: text/plain")
        print()
        print(f"CGI Script Error: {str(e)}", file=sys.stderr)