while true; do
  curl -s -o /dev/null -w "\nHTTP Status: %{http_code}\n" \
    -H "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36" \
    -H "Accept: application/json, text/plain, */*" \
    -H "Accept-Encoding: gzip, deflate, br" \
    -H "Accept-Language: en-US,en;q=0.9" \
    -H "Cache-Control: no-cache" \
    -H "Connection: keep-alive" \
    -H "Host: localhost:8080" \
    -H "Pragma: no-cache" \
    -H "Upgrade-Insecure-Requests: 1" \
    -H "DNT: 1" \
    -H "Referer: http://localhost:8080/" \
    -H "Origin: http://localhost:8080" \
    -H "X-Requested-With: XMLHttpRequest" \
    -H "Authorization: Bearer YOUR_ACCESS_TOKEN" \
    http://localhost:8080/cgi-bin/script.py
done
