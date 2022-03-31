import logging
import os
import socketserver
import socket
import time
import sys
import threading
from threading import Thread
from datetime import date, datetime
import paramiko
import binascii
from logging import basicConfig, getLogger, StreamHandler, DEBUG
from __init__ import get_module_logger
from tcp_server import TCPServer
from unix_server import UnixServer
import task_variable
import datetime
import secrets

sock_file = "/var/run/iot_events"
server_ip = "0.0.0.0"
server_port = 8080
listen_num = 5
buffer_size = 1024

flag = False

# task_variable.iot_deviceに対してランダムにハートビートを行う


class Heartbeat(Thread):
    def __init__(self):
        self.logger = get_module_logger(__name__)
        Thread.__init__(self)

    def run(self):
        while True:
            key_command = "ps " + secrets.token_hex(16)  # 正常を確認するコマンド識別用
            reset_command = ";optee_example_watchdog"  # WDT-PTAを呼び出すコマンド
            command = key_command + reset_command
            while task_variable.iot_device:
                target_ip = task_variable.iot_device.pop()
                self.register_task(target_ip, key_command)
                self.execute_command(target_ip, 'root', 'toor', command)

            self.check_expire()
            time.sleep(1)

    def register_task(self, target_ip, command):
        self.logger.debug('append task {0} {1}'.format(target_ip, command))
        register_time = datetime.datetime.now()
        expire_time = register_time + datetime.timedelta(seconds=180)
        task_variable.executing_tasks.append(task_variable.Task(
            register_time, expire_time, target_ip, command))

    def execute_command(self, client_ip, user, password, command):
        try:
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(client_ip, username=user, password=password)

            stdin, stdout, stderr = ssh.exec_command(command)
        except paramiko.SSHException as e:
            ssh.close()
            self.logger.debug("ssh connection failed: %s", e)

    # Heart beatに成功した場合，新しいコマンドとexpire timeの延長を行う
    def updated_task(self, target_task):
        target_task.heartbeat_check = False
        key_command = "touch /tmp/" + secrets.token_hex(16)
        reset_command = ";optee_example_watchdog"
        command = key_command + reset_command
        self.execute_command(target_task.ip_address, 'root', 'toor', command)
        target_task.expire_time = datetime.datetime.now() + datetime.timedelta(seconds=15)
        target_task.execute_cmd = command
        self.logger.debug('update task: next expire time is {0}, execute cmd:{1}'.format(
            target_task.expire_time, target_task.execute_cmd))

    # executing_taskの値がexpireしてないかを確認
    def check_expire(self):
        for i in task_variable.executing_tasks:
            if i.heartbeat_check:
                self.logger.debug("flag is on")
                self.updated_task(i)

            # 現在の時刻がexpire timeを過ぎていた場合，heart beatのリストから外す
            if i.expire_time <= datetime.datetime.now():
                self.logger.debug(
                    '{0} task time expired!!'.format(i.ip_address))
                task_variable.executing_tasks.remove(i)


if __name__ == "__main__":
    TCPServer(server_ip, server_port, '',  timeout=86400, debug=False).start()
    Heartbeat().start()

    if os.path.exists(sock_file):
        os.remove(sock_file)
    UnixServer(sock_file).start()
