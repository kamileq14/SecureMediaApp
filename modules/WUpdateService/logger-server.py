import socket
import threading
import logging
import datetime
import signal
import sys
import os
from colorama import init, Fore, Style

# Inicjalizacja kolorów
init(autoreset=True)

# Konfiguracja logowania
logger = logging.getLogger()
logger.setLevel(logging.INFO)

file_handler = logging.FileHandler('logger-log.txt', encoding='utf-8')
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
console_formatter = ColorFormatter('%(asctime)s - %(levelname)s - %(message)s')
console_handler.setFormatter(console_formatter)

logger.addHandler(file_handler)
logger.addHandler(console_handler)

HOST = '0.0.0.0'
PORT = server_port
PASSWORD = "server_password"
clients = []
clients_lock = threading.Lock()
running = True  # flaga do kontrolowania pętli

def close_screen_session():
    screen_session = os.environ.get('STY')
    if screen_session:
        logger.info(f"Zamykanie sesji screen: {screen_session}")
        os.system(f'screen -S {screen_session} -X quit')

def handle_client(client_socket, client_address):
    logger.info(f"Połączenie z {client_address}")
    with clients_lock:
        clients.append(client_socket)

    try:
        received = client_socket.recv(1024).decode('utf-8').strip()
        if received != PASSWORD:
            logger.warning(f"Odrzucono klienta {client_address}: nieprawidłowe hasło")
            client_socket.settimeout(10)
            client_socket.sendall(b"AUTH_FAIL\n")
            client_socket.close()
            with clients_lock:
                clients.remove(client_socket)
            return
        else:
            client_socket.sendall(b"AUTH_OK\n")
            logger.info(f"Klient {client_address} uwierzytelniony")

        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            message = data.decode('utf-8', errors='replace').strip()
            if message and message != "[PING]":
                timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                log_line = f"[{timestamp}] {client_address}: {message}"
                logger.info(log_line)
    except Exception as e:
        logger.error(f"Błąd klienta {client_address}: {e}")
    finally:
        with clients_lock:
            if client_socket in clients:
                clients.remove(client_socket)
        client_socket.close()
        logger.info(f"Rozłączono klienta {client_address}")

def signal_handler(sig, frame):
    global running
    logger.info("Zamykanie serwera...")
    running = False
    with clients_lock:
        for c in clients:
            try:
                c.sendall(b"DISCONNECT\n")
                c.shutdown(socket.SHUT_RDWR)
                c.close()
            except:
                pass
        clients.clear()
    close_screen_session()

def stdin_listener():
    while True:
        line = sys.stdin.readline()
        if line.strip().lower() == 'exit':
            signal_handler(signal.SIGINT, None)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

threading.Thread(target=stdin_listener, daemon=True).start()

def start_server():
    global running
    clients.clear()
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)
    server.settimeout(1.0)
    logger.info(f"Serwer nasłuchuje na {HOST}:{PORT}")

    while running:
        try:
            client_socket, client_address = server.accept()
            thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
            thread.daemon = True
            thread.start()
        except socket.timeout:
            continue
        except Exception as e:
            logger.error(f"Błąd serwera: {e}")

    server.close()
    logger.info("Serwer zakończył działanie.")

if __name__ == "__main__":
    start_server()
