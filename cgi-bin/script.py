from flask import Flask, request, Response

app = Flask(__name__)

@app.route("/", methods=["GET"])
def upload_form():
    # Simple HTML form for manual testing (optional)
    return """
    <!doctype html>
    <html><body>
      <h2>Send an image via Postman to /upload</h2>
      <form action="/upload" method="post" enctype="multipart/form-data">
        <input type="file" name="image" accept="image/*" required>
        <button type="submit">Upload & View</button>
      </form>
    </body></html>
    """

@app.route("/upload", methods=["POST"])
def upload_image():
    # 1. Ensure the 'image' part is in the form-data
    if "image" not in request.files:
        return "Field 'image' not found in request.", 400

    file = request.files["image"]
    # 2. Ensure a file was selected
    if file.filename == "":
        return "No file selected.", 400

    # 3. Read raw bytes and determine MIME
    img_bytes = file.read()
    mime_type = file.content_type or "application/octet-stream"

    # 4. Return the image bytes with correct header
    return Response(img_bytes, mimetype=mime_type)

if __name__ == "__main__":
    app.run(debug=True)
