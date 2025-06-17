import socket
import threading
import logging
import datetime
import signal
import sys
from colorama import init, Fore, Style

init(autoreset=True)

logger = logging.getLogger()
logger.setLevel(logging.INFO)

file_handler = logging.FileHandler('logger-log.txt', encoding='utf-8')
file_handler.setLevel(logging.INFO)
file_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
file_handler.setFormatter(file_formatter)

class ColorFormatter(logging.Formatter):
    LEVEL_COLORS = {
        logging.INFO: Fore.GREEN,
        logging.WARNING: Fore.YELLOW,
        logging.ERROR: Fore.RED,
        logging.DEBUG: Fore.CYAN
    }
    def format(self, record):
        color = self.LEVEL_COLORS.get(record.levelno, "")
        return f"{color}{super().format(record)}{Style.RESET_ALL}"

console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
console_formatter = ColorFormatter('%(asctime)s - %(levelname)s - %(message)s')
console_handler.setFormatter(console_formatter)

logger.addHandler(file_handler)
logger.addHandler(console_handler)

HOST = '0.0.0.0'
PORT = 25565
clients = []

def handle_client(client_socket, client_address):
    logger.info(f"Nowe połączenie od {client_address}")
    clients.append(client_socket)
    try:
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            message = data.decode('utf-8', errors='replace').strip()
            if message:
                timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                log_line = f"[{timestamp}] {client_address}: {message}"
                logger.info(log_line)
    except Exception as e:
        logger.error(f"Błąd klienta {client_address}: {e}")
    finally:
        if client_socket in clients:
            clients.remove(client_socket)
        client_socket.close()
        logger.info(f"Rozłączono klienta {client_address}")

def signal_handler(sig, frame):
    logger.info("Zamykanie serwera...")
    for c in clients:
        try:
            c.sendall(b"DISCONNECT\n")
            c.shutdown(socket.SHUT_RDWR)
            c.close()
        except:
            pass
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)
    logger.info(f"Serwer nasłuchuje na {HOST}:{PORT}")

    while True:
        try:
            client_socket, client_address = server.accept()
            thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
            thread.daemon = True
            thread.start()
        except Exception as e:
            logger.error(f"Błąd serwera: {e}")

if __name__ == "__main__":
    start_server()
