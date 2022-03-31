from collections import deque
import datetime
import dataclasses

iot_device = deque([])
buf = []
executing_tasks = []


@dataclasses.dataclass
class Task:
    create_time: datetime.datetime
    expire_time: datetime.datetime
    ip_address: str
    execute_cmd: str
    heartbeat_check: bool = False
