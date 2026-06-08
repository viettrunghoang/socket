import os
import urllib.parse
import mimetypes
from http.server import BaseHTTPRequestHandler, HTTPServer

class FileServerHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Giải mã URL và chặn directory traversal (bảo mật cơ bản)
        decoded_path = urllib.parse.unquote(self.path)
        safe_path = decoded_path.lstrip('/')
        os_path = os.path.join(os.getcwd(), safe_path)

        # Kiểm tra đường dẫn có hợp lệ hay không
        if not os.path.exists(os_path):
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"404 Not Found")
            return

        # 1. NẾU LÀ THƯ MỤC
        if os.path.isdir(os_path):
            self.send_response(200)
            self.send_header("Content-type", "text/html; charset=utf-8")
            self.end_headers()
            
            html = f"""<!DOCTYPE html><html><head><meta charset='utf-8'>
            <title>Index of {decoded_path}</title></head><body>
            <h2>Index of {decoded_path}</h2><hr><ul>"""
            
            if decoded_path != '/':
                html += "<li><b><a href='../'>../ (Lên một cấp)</a></b></li>"
                
            try:
                items = os.listdir(os_path)
                # Sắp xếp folder lên trước, file xuống sau cho đẹp
                items.sort(key=lambda x: os.path.isfile(os.path.join(os_path, x)))
                
                for item in items:
                    full_item_path = os.path.join(os_path, item)
                    link = os.path.join(decoded_path, item).replace('\\', '/')
                    
                    if os.path.isdir(full_item_path):
                        # Thư mục: In đậm <b>
                        html += f"<li><b><a href='{link}/'>📁 {item}/</a></b></li>"
                    else:
                        # File: In nghiêng <i>
                        html += f"<li><i><a href='{link}'>📄 {item}</a></i></li>"
                        
            except PermissionError:
                html += "<li><i>Không có quyền truy cập thư mục này.</i></li>"
                
            html += "</ul><hr></body></html>"
            self.wfile.write(html.encode('utf-8'))

        # 2. NẾU LÀ FILE
        elif os.path.isfile(os_path):
            # Đoán định dạng file (video, audio, image, text)
            mime_type, _ = mimetypes.guess_type(os_path)
            if mime_type is None:
                mime_type = 'application/octet-stream'

            try:
                with open(os_path, 'rb') as f:
                    file_stat = os.stat(os_path)
                    self.send_response(200)
                    self.send_header("Content-type", mime_type)
                    self.send_header("Content-Length", str(file_stat.st_size))
                    self.end_headers()
                    
                    # Gửi theo từng chunk để hỗ trợ stream video lớn
                    chunk_size = 8192
                    while chunk := f.read(chunk_size):
                        self.wfile.write(chunk)
            except IOError:
                self.send_response(500)
                self.end_headers()
                self.wfile.write(b"500 Internal Server Error")

def run_server(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, FileServerHandler)
    print(f"Server Python (Fake C++) dang chay tai: http://localhost:{port}")
    print(f"Thu muc hien tai: {os.getcwd()}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nTat server.")
        httpd.server_close()

if __name__ == '__main__':
    run_server()