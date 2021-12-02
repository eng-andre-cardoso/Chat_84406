CC = gcc
CCrasp = ~/buildroot/buildroot-2021.08/output/host/bin/./aarch64-buildroot-linux-gnu-gcc

IP = 10.42.0.203
#IP = 130.10.0.1
#PORT = 5001
PORT = 5000
ADDR = 0.0.0.0  


DIR = /home/mota/Embebidos-1semestre/Class_Code/DDrivers/led/led.ko

# Compile Server to Host
server: 
	$(CC) Server.c -o server.elf -lpthread

# Compile Server to Rasp
embServer:
	$(CCrasp) Server.c -o embServer.elf -lpthread
	scp embServer.elf root@$(IP):/embebidos/SerCli

#Run Server	
runserver:	
	./server.elf $(PORT)
	




#----------------------------------------------------------------------

#Compile Client to Host
client: 
	$(CC) Client.c -o client.elf -lpthread -lrt

#Compile Client to Rasp
embClient: 
	$(CCrasp) Client.c -o embClient.elf -lpthread -lrt
	scp embClient.elf root@$(IP):/embebidos/SerCli

#Run Client
runclient:
	./client.elf $(ADDR) $(PORT)
	
#----------------------------------------------------------------------

#Compile Send Out

send_out:
	$(CC) client_send.c -o send_out.elf -lpthread -lrt


#Run Send Out
run send_out
	./send_out.elf "teste de envio"

#--------------------------------------------------------------------



#Copy Device Driver to Rasp
sendDriver:
	scp $(DIR) root@$(IP):/embebidos/SerCli


#Install Device Driver
installDriver:
	#ssh root@$(IP)
	#cd /embebidos
	insmod led.ko
	#lsmod


#----------------------------------------------------------------------

#Compile send (will send via Posix Message Queue to client service)
clientsend: 
	$(CC) client_send.c -o send.out -lpthread -lrt

send:
	./send.out "Predefined message"



clean: 
	rm -f  *.elf *.out
	
stat: 
	netstat -ta
	
transfer: 
	scp client.elf send.out root@$(IP):/etc
	
open: 
	ssh root@$(IP)
