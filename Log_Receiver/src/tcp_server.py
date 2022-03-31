"""
Copyright (c) 2020 Matthew Wachter
Released under the MIT LICENSE
https://github.com/matthewwachter/py-tcp-threaded-server/blob/master/LICENSE
"""

import logging
import socket
from threading import Thread, Timer
from datetime import datetime
from collections import deque
import binascii
from logging import basicConfig, exception, getLogger, StreamHandler, DEBUG
from __init__ import get_module_logger
from unix_server import UnixServer
import task_variable
import log_buffer


def format_audit_msg(audit_type, msg):
    if ('audit' not in msg):
        return None
    msg = msg.split(',')
    return '{} type={} {}\n'.format(msg[-1], audit_type, msg[0])


def translate_audit_log(raw_log):
    if ('syscall' in raw_log):
        return format_audit_msg('SYSCALL', raw_log)
    elif ('proctitle' in raw_log):
        return format_audit_msg('PROCTITLE', raw_log)
    elif ('saddr' in raw_log):
        return format_audit_msg('SOCKADDR', raw_log)
    elif ('cwd' in raw_log):
        return format_audit_msg('CWD', raw_log)
    elif ('item' in raw_log):
        return format_audit_msg('PATH', raw_log)
    elif ('op=start' in raw_log):
        return format_audit_msg('DAEMON_START', raw_log)
    elif ('op=add_rule' in raw_log):
        return format_audit_msg('CONFIG_CHANGE', raw_log)
    return None


class TCPServer(Thread):
    def __init__(self, host, port, audit_buffer, timeout=60, debug=False):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.debug = debug
        self.logger = get_module_logger(__name__)
        self.audit_buffer = deque(maxlen=1000)
        self.audit_buffer_hearbeat = deque(maxlen=1000)

        Thread.__init__(self)

    def run(self):
        self.logger.debug('TCP SERVER Starting...')
        self.listen()

    def listen(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        self.sock.bind((self.host, self.port))
        self.logger.debug(
            'TCP SERVER Socket Bound {0} {1}'.format(self.host, self.port))

        # start listening for a client
        self.sock.listen(30)
        self.logger.debug('TCP SERVER Listening...')
        while True:
            client, address = self.sock.accept()
            client.settimeout(self.timeout)

            # デバイスをheartbeatの監視対象に設定
            if next((target_ip for target_ip in task_variable.executing_tasks if target_ip.ip_address == address[0]), None) is None:
                if address[0] not in task_variable.iot_device:
                    task_variable.iot_device.append(address[0])
                    self.logger.debug(
                        'CLIENT Connected: {0}'.format(address[0]))

            # start a thread to listen to the client
            Thread(target=self.listenToClient,
                   args=(client, address[0])).start()
            Thread(target=self.save_audit_log).start()
            Thread(target=self.logmatch).start()

    def listenToClient(self, client, address):
        size = 26000
        end_code = "zerozero"

        while True:
            try:
                # try to receive data from the client
                flag = False
                data = client.recv(size).decode('utf-8')
                data_length = len(data)
                # check packet size
                if data_length == 0:
                    raise Exception()
                self.parseEventLog(data)

            except:
                client.close()
                flag = True
                return False

    def parseEventLog(self, data):
        for d in data.splitlines():
            audit_log = translate_audit_log(d)

            if audit_log is not None:
                self.audit_buffer.append(audit_log)
                self.audit_buffer_hearbeat.append(audit_log)
                log_buffer.audit.append(audit_log)

    def save_audit_log(self):
        temp = ""
        save_audit_log_name = "TEE-PA_" + datetime.now().strftime("%m.%d.%Y-%H") + ".log"
        if len(self.audit_buffer) > 5:
            while(len(self.audit_buffer) > 10):
                temp += self.audit_buffer.popleft()

            with open(save_audit_log_name, "w") as f:
                f.write(temp)
        save_audit_log_periodically_execute = Timer(0.5, self.save_audit_log)
        save_audit_log_periodically_execute.start()

    def logmatch(self):
        proctitle = "proctitle="
        #for i in task_variable.executing_tasks:
        #    self.logger.debug('{0} {1} {2} {3}'.format(
        #        i.create_time, i.expire_time, i.ip_address, i.execute_cmd))

        #matching_ip = next(
        #    (target_ip for target_ip in task_variable.executing_tasks if target_ip.ip_address == address), None)
        #self.logger.debug(matching_ip.__dict__)

        # ログの中にコマンドが存在するかの確認
        while len(self.audit_buffer_hearbeat) > 10:
            audit_log = self.audit_buffer_hearbeat.popleft()
            if proctitle in audit_log:
                try:
                    idx = audit_log.find(proctitle)
                    audit_proctilte = audit_log[idx+len(proctitle):].strip()
                    execute_cmd = binascii.a2b_hex(
                        audit_proctilte).decode('utf-8')
                    execute_cmd = execute_cmd.replace('\x00', ' ')
                except:
                    execute_cmd = audit_proctilte

                # 実行コマンドがHearbeatタスクに入っているのかを確認
                for i in task_variable.executing_tasks:
                    if execute_cmd in i.execute_cmd:
                        #self.logger.debug(i.ip_address)
                        i.heartbeat_check = True
        logmatch = Timer(1, self.logmatch)
        logmatch.start()
