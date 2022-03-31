import socket
from threading import Thread, Timer
from __init__ import get_module_logger
import log_buffer


class UnixServer(Thread):
    """
    Start unix server and pass logs to SPADE
    """

    def __init__(self, socket_path):
        self.socket_path = socket_path
        self.logger = get_module_logger(__name__)
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        Thread.__init__(self)

    def run(self):
        self.listen()

    def listen(self):
        self.logger.debug("Unix Socket Server Listening")
        self.sock.bind(self.socket_path)
        self.sock.listen(1)

        while True:
            connection, address = self.sock.accept()
            self.logger.debug("client connectd on unix socker server")
            Thread(target=self.listen_client, args=(
                connection, address)).start()

    def listen_client(self, connection, address):
        # clientの状態を確認して適宜portをcloseする
        temp = ""
        while True:
            if len(log_buffer.audit) > 30:
                temp = log_buffer.audit.popleft()
                connection.send(temp.encode('utf-8'))
