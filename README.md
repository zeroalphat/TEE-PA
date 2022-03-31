
## Evaluation Platform  
We have tested and confirmed that  works on the following platforms.

- Raspberry Pi3 Model B (Arm Cortex-A53, 4 cores, 1.2 GHz,1GB memory, 16GB SD card) or QEMU v8-arm
- Linux kernel 5.4 
- Linux Audit enabled in kernel options

## Set up the enviroment

The following commands must be installed.
- [soc_term](https://github.com/linaro-swg/soc_term)
- Docker
- Docker Compose

## Project build 

Run `docker-compose build --no-cache` command to build the required software.

## Run applications

Run `docker-compose up -d` command to start the container.
Then run the
`docker-compose exec spade /app/init-spade.sh`
command to perform the initial SPADE configuration.
The terminal that makes the connection to QEMU is running as part of Docker Compose and must be attached from the host to the terminal container.
Therefore, run the `docker attach ree-terminal` command to connect to the terminal.
Once the connection is made to the terminal, you can log in to the Linux shell running on QEMU.
Please refer to the OP-TEE documentation for information on performing a TA.

## Usage

The following is an example of the settings required to operate TA-Collect and a simple operation check.
First, configure rules for Linux Audit. This configuration must be done using the auditctl command.
Also, since TEE-PA needs to exclude tee-supplicant system calls, configure Linux Audit rules to exclude tee-supplicant PID.
- Run the `TEE_SUPP_PID=$(pidof tee-supplicant)` command to obtain the PID of the tee-supplicant
- Run the `auditctl -a exit,always -F arch=aarch64 -S read -S readv -S write -S writev -S kill -S exit -S exit_group -S connect -S sendto -S recvfrom -S sendmsg -S recvmsg -S mmap -S linkat -S symlinkat -S execve -S close -S openat -S dup -S bind -S accept -S accept4 -S renameat -S memfd_create -F pid!=$TEE_SUPP_PID` command to configure the audit rules

Logs sent from Linux are stored in SPADE via Log Receiver.
SPADE can examine the sent logs by issuing a qeury.
- Running `docker-compose exec spade bash` to get inside the SPADE container
- Run the `./bin/spade query` command to start spade's query client
- Run the `set storage Neo4j` command to configure storage settings
- Run the following command to retrieve all processes from the logs
```bash
%only_processes = "type" == 'Process'
dump $base.getVertex(%only_processes)
```

The following is an overview of how to perform Provenance Auditing using SPADE.
- Issue a query to collect processes named wget
```bash
%wget = "name" == 'wget'
$wget_processes = $base.getVertex(%wget)
```
- Issue a query that examines the processes and files that are causally related to the process named wget
```bash
$dag = $base.getLineage($wget_processes, 10, 'both')
```
- Save the results of the query execution as a dot file
```bash
export > /tmp/dag.dot
visualize force $dag
```
`docker cp spade:/tmp/dag.dot /tmp/dag.dot` command can be used to copy files from the container to the host.  
Run the `dot -Ksfdp -o <outputname> -Tsvg /tmp/dag.dot` command can convert dot to svg file.

## Limitations 

Hardware based functions are not supported.
- Kernel Integrity check
- PTA-WDT: Performing a system reset using the watchdog timer

## License 
- Linux version 4.14 LICENSE GPLv2
- SPADE version 8d51f11a7a5aa7db4fa5ddabcf13e672881f2ab6 GPLv3
- OP-TEE version 3.11.0 LICENSE GPLv2
- TA LICENSE MIT
- Log Receiver LICENSE MIT
