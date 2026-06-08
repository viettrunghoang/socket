from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
import cgi

class CalculatorHandler(BaseHTTPRequestHandler):
    def handle_calculation(self, params):
        a_str = params.get('a', [''])[0]
        b_str = params.get('b', [''])[0]
        op = params.get('op', [''])[0]

        if not a_str or not b_str or not op:
            return "<h2>Vui lòng cung cấp đủ tham số (a, b, op)</h2>"

        try:
            a, b = float(a_str), float(b_str)
            if op == 'add': result, symbol = a + b, '+'
            elif op == 'sub': result, symbol = a - b, '-'
            elif op == 'mul': result, symbol = a * b, '*'
            elif op == 'div':
                if b == 0: return "<h2>Lỗi: Không thể chia cho 0!</h2>"
                result, symbol = a / b, '/'
            else:
                return "<h2>Toán tử không hợp lệ! Dùng add, sub, mul, div.</h2>"
            
            return f"<h2>Kết quả phép tính:</h2><h3>{a} {symbol} {b} = {result}</h3>"
        except ValueError:
            return "<h2>Lỗi: Toán hạng phải là số!</h2>"

    def send_html_response(self, content):
        self.send_response(200)
        self.send_header('Content-type', 'text/html; charset=utf-8')
        self.end_headers()
        html = f"""
        <!DOCTYPE html>
        <html>
        <head><meta charset="utf-8"><title>Calculator</title></head>
        <body style='font-family: Arial; text-align: center; margin-top: 50px;'>
            {content}
        </body>
        </html>
        """
        self.wfile.write(html.encode('utf-8'))

    def do_GET(self):
        parsed_path = urlparse(self.path)
        params = parse_qs(parsed_path.query)
        result_content = self.handle_calculation(params)
        self.send_html_response(result_content)

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        params = parse_qs(post_data)
        result_content = self.handle_calculation(params)
        self.send_html_response(result_content)

def run(server_class=HTTPServer, handler_class=CalculatorHandler, port=8080):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Server Python dang chay tren cong {port} (Fake C++)...")
    httpd.serve_forever()

if __name__ == '__main__':
    run()