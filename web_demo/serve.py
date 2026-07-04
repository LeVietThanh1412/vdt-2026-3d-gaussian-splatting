#!/usr/bin/env python3
import http.server
import json
import os
import socketserver
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent
PORT = int(os.environ.get("PORT", "8000"))

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        # Route requests starting with /data/ directly to project root data directory
        if path.startswith("/data/"):
            relative_path = path[len("/data/"):]
            return os.path.join(PROJECT_ROOT, "data", relative_path)
        return super().translate_path(path)

    def do_GET(self):
        # API endpoint to list all .glb files in data/output/
        if self.path == "/api/models":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

            output_dir = PROJECT_ROOT / "data" / "output"
            models = []
            if output_dir.exists():
                models = [f.name for f in output_dir.iterdir() if f.suffix.lower() == ".glb"]
            self.wfile.write(json.dumps(models).encode("utf-8"))
        else:
            super().do_GET()

with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
    print(f"=== 3DGS AR Web Server ===")
    print(f" -> Local Host      : http://localhost:{PORT}/web_demo/")
    print(f" -> Network Access  : http://<your-lan-ip>:{PORT} (for mobile AR)")
    print(f" -> Project Folder  : {PROJECT_ROOT}")
    print("==========================")
    httpd.serve_forever()
